#include "eqnpch.h"
#include "equinox/graphics/GfxOpenGLBackend.h"
#include "equinox/graphics/GfxRenderer.h"
#include "equinox/core/Log.h"
#include "equinox/core/Profiler.h"
#include "equinox/renderer/Mesh.h"
#include "equinox/renderer/Texture.h"

#include <glad/glad.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#include <chrono>
#include <cstring>
#include <fstream>
#include <vector>

// ---------------------------------------------------------------------------
//  Constants / entry points that ARB_gl_spirv added (GL 4.6).
//  They are loaded dynamically so that the backend does not depend on the
//  version of glad that was generated.
// ---------------------------------------------------------------------------
#ifndef GL_SHADER_BINARY_FORMAT_SPIR_V
#define GL_SHADER_BINARY_FORMAT_SPIR_V 0x9551
#endif

#define EQN_GL_LOWER_LEFT          0x8CA1
#define EQN_GL_UPPER_LEFT          0x8CA2
#define EQN_GL_NEGATIVE_ONE_TO_ONE 0x935E
#define EQN_GL_ZERO_TO_ONE         0x935F

namespace Equinox::Gfx
{
	using PFN_glShaderBinary = void (*)(GLsizei count, const GLuint* shaders, GLenum format, const void* binary, GLsizei length);
	using PFN_glSpecializeShader = void (*)(GLuint shader, const char* entryPoint, GLuint numSpecializationConstants, const GLuint* indices, const GLuint* values);
	using PFN_glClipControl = void (*)(GLenum origin, GLenum depth);

	static PFN_glShaderBinary     s_glShaderBinary = nullptr;
	static PFN_glSpecializeShader s_glSpecializeShader = nullptr;
	static PFN_glClipControl      s_glClipControl = nullptr;

	static double NowMs()
	{
		using namespace std::chrono;
		return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
	}

	static std::string ResolveShaderPath(const std::string& fileName)
	{
		fs::path dir = fs::current_path();
		for (int level = 0; level < 6; level++)
		{
			const fs::path candidates[] = {
				dir / "assets" / "shaders" / fileName,
				dir / "equinox" / "assets" / "shaders" / fileName,
			};
			for (const auto& c : candidates)
			{
				std::error_code ec;
				if (fs::exists(c, ec)) return c.string();
			}
			if (!dir.has_parent_path() || dir.parent_path() == dir) break;
			dir = dir.parent_path();
		}
		return {};
	}

	static std::vector<char> ReadBinaryFile(const std::string& path)
	{
		std::ifstream file(path, std::ios::ate | std::ios::binary);
		if (!file.is_open()) return {};

		size_t size = (size_t)file.tellg();
		std::vector<char> buffer(size);
		file.seekg(0);
		file.read(buffer.data(), (std::streamsize)size);
		return buffer;
	}

	static std::string ReadTextFile(const std::string& path)
	{
		std::ifstream file(path, std::ios::in | std::ios::binary);
		if (!file.is_open()) return {};
		return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	}

	// Init / Shutdown
	void OpenGLBackend::Init(void* windowHandle, u32 width, u32 height, bool vsync)
	{
		m_Window = windowHandle;
		m_WindowWidth = width;
		m_WindowHeight = height;

		// The caller (GfxRenderer::Init) already created the legacy GL renderer:
		// the GL context is current and glad has been loaded.
		LoadExtraFunctions();

		// ---- informations shown by the "GPU" panel ----
		const char* renderer = (const char*)glGetString(GL_RENDERER);
		const char* version = (const char*)glGetString(GL_VERSION);
		const char* vendor = (const char*)glGetString(GL_VENDOR);
		m_GPUInfo.device = renderer ? renderer : "OpenGL";
		m_GPUInfo.apiVersion = version ? version : "OpenGL";
		m_GPUInfo.driver = vendor ? vendor : "";
		m_GPUInfo.maxAnisotropy = 1.0f;
		m_GPUInfo.timestamps = false;      // glQueryCounter_timestamps could be added later
		m_GPUInfo.multiViewport = true;    // ImGui can detach windows in OpenGL

		// Same "look" as Vulkan: the window is not multisampled there either.
		glDisable(GL_MULTISAMPLE);

		CreateShaderProgram();
		CreateWhiteTexture();
		CreateUniformBuffers();
		CreateSceneTarget(width, height);
		m_PendingWidth = m_TargetWidth;
		m_PendingHeight = m_TargetHeight;

		SetVSync(vsync);

		EQN_CORE_INFO("OpenGL backend ready ({0} / {1})", m_GPUInfo.device, m_GPUInfo.apiVersion);
	}

	void OpenGLBackend::Shutdown()
	{
		DestroyObjects();
	}

	void OpenGLBackend::LoadExtraFunctions()
	{
		s_glShaderBinary = (PFN_glShaderBinary)glfwGetProcAddress("glShaderBinary");
		s_glSpecializeShader = (PFN_glSpecializeShader)glfwGetProcAddress("glSpecializeShader");
		s_glClipControl = (PFN_glClipControl)glfwGetProcAddress("glClipControl");

		m_SpirVSupported = (s_glShaderBinary != nullptr) && (s_glSpecializeShader != nullptr);

		if (m_SpirVSupported)
			EQN_CORE_INFO("OpenGL: ARB_gl_spirv available (shaders loaded from SPIR-V, like Vulkan)");
		else
			EQN_CORE_WARN("OpenGL: glShaderBinary/glSpecializeShader not available; "
				"the same .vert/.frag sources will be compiled as GLSL 4.50 instead");
	}

	void OpenGLBackend::SetVSync(bool vsync)
	{
		glfwSwapInterval(vsync ? 1 : 0);
	}

	void OpenGLBackend::OnWindowResize(u32 width, u32 height)
	{
		m_WindowWidth = width;
		m_WindowHeight = height;
	}

	// Shaders: SPIR-V first (identical to Vulkan), GLSL source as a fallback
	OpenGLBackend::GLObject OpenGLBackend::CompileSpirVStage(const std::string& spvPath, u32 glStage, const char* debugName)
	{
		std::vector<char> code = ReadBinaryFile(spvPath);
		if (code.empty())
		{
			EQN_CORE_ERROR("OpenGL: cannot read '{0}' (run CompileShaders.bat/sh)", spvPath);
			return 0;
		}

		GLuint shader = glCreateShader(glStage);
		s_glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, code.data(), (GLsizei)code.size());
		s_glSpecializeShader(shader, "main", 0, nullptr, nullptr);

		GLint status = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE)
		{
			GLint length = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			std::string log((size_t)(length > 1 ? length : 1), '\0');
			if (length > 1) glGetShaderInfoLog(shader, length, nullptr, log.data());
			EQN_CORE_ERROR("OpenGL: SPIR-V shader '{0}' failed: {1}", debugName, log);
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	OpenGLBackend::GLObject OpenGLBackend::CompileGLSLStage(const std::string& sourcePath, u32 glStage, const char* debugName)
	{
		std::string source = ReadTextFile(sourcePath);
		if (source.empty())
		{
			EQN_CORE_ERROR("OpenGL: cannot read '{0}'", sourcePath);
			return 0;
		}

		{
			size_t pos = 0;
			while ((pos = source.find("set = 0, ", pos)) != std::string::npos)
				source.erase(pos, 9);
		}

		GLuint shader = glCreateShader(glStage);
		const char* src = source.c_str();
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		GLint status = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE)
		{
			GLint length = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			std::string log((size_t)(length > 1 ? length : 1), '\0');
			if (length > 1) glGetShaderInfoLog(shader, length, nullptr, log.data());
			EQN_CORE_ERROR("OpenGL: GLSL shader '{0}' failed: {1}", debugName, log);
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	bool OpenGLBackend::CreateShaderProgram()
	{
		GLObject vert = 0;
		GLObject frag = 0;

		if (m_SpirVSupported)
		{
			// SPIR-V compiled for OpenGL (-G) from the very same files as Vulkan
			const std::string vertPath = ResolveShaderPath("spv/gl/mesh.vert.spv");
			const std::string fragPath = ResolveShaderPath("spv/gl/mesh.frag.spv");
			if (!vertPath.empty()) vert = CompileSpirVStage(vertPath, GL_VERTEX_SHADER, "mesh.vert.spv");
			if (!fragPath.empty()) frag = CompileSpirVStage(fragPath, GL_FRAGMENT_SHADER, "mesh.frag.spv");
		}

		if (!vert || !frag)
		{
			// Fallback: the same GLSL sources (they compile as GLSL 4.50 too)
			const std::string vertSrc = ResolveShaderPath("mesh.vert");
			const std::string fragSrc = ResolveShaderPath("mesh.frag");
			if (!vert && !vertSrc.empty()) vert = CompileGLSLStage(vertSrc, GL_VERTEX_SHADER, "mesh.vert");
			if (!frag && !fragSrc.empty()) frag = CompileGLSLStage(fragSrc, GL_FRAGMENT_SHADER, "mesh.frag");
		}

		if (!vert || !frag)
		{
			if (vert) glDeleteShader(vert);
			if (frag) glDeleteShader(frag);
			EQN_CORE_CRITICAL("OpenGL: cannot build the scene shader program");
			return false;
		}

		m_Program = glCreateProgram();
		glAttachShader(m_Program, vert);
		glAttachShader(m_Program, frag);
		glLinkProgram(m_Program);

		glDeleteShader(vert);
		glDeleteShader(frag);

		GLint status = GL_FALSE;
		glGetProgramiv(m_Program, GL_LINK_STATUS, &status);
		if (status != GL_TRUE)
		{
			GLint length = 0;
			glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &length);
			std::string log((size_t)(length > 1 ? length : 1), '\0');
			if (length > 1) glGetProgramInfoLog(m_Program, length, nullptr, log.data());
			EQN_CORE_ERROR("OpenGL: program link failed: {0}", log);
			glDeleteProgram(m_Program);
			m_Program = 0;
			return false;
		}
		return true;
	}

	// Buffers / textures / render target
	void OpenGLBackend::CreateWhiteTexture()
	{
		const unsigned char pixel[4] = { 255, 255, 255, 255 };

		glGenTextures(1, &m_WhiteTexture);
		glBindTexture(GL_TEXTURE_2D, m_WhiteTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void OpenGLBackend::CreateUniformBuffers()
	{
		// The dynamic offset used for the per-object block must be aligned.
		GLint alignment = 0;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
		m_ObjectStride = alignment > (GLint)kObjectUniformAlignment ? (u32)alignment : kObjectUniformAlignment;

		glGenBuffers(1, &m_SceneUbo);
		glBindBuffer(GL_UNIFORM_BUFFER, m_SceneUbo);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneUniform), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_SceneUbo);   // binding 0 = SceneUBO in the shader

		glGenBuffers(1, &m_ObjectUbo);
		glBindBuffer(GL_UNIFORM_BUFFER, m_ObjectUbo);
		glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)m_ObjectStride * MaxObjectsPerFrame, nullptr, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void OpenGLBackend::DestroyUniformBuffers()
	{
		if (m_SceneUbo) { glDeleteBuffers(1, &m_SceneUbo); m_SceneUbo = 0; }
		if (m_ObjectUbo) { glDeleteBuffers(1, &m_ObjectUbo); m_ObjectUbo = 0; }
	}

	void OpenGLBackend::CreateSceneTarget(u32 width, u32 height)
	{
		DestroySceneTarget();

		m_TargetWidth = width;
		m_TargetHeight = height;

		// Color: RGBA8 = VK_FORMAT_R8G8B8A8_UNORM on the Vulkan side
		glGenTextures(1, &m_ColorTexture);
		glBindTexture(GL_TEXTURE_2D, m_ColorTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenRenderbuffers(1, &m_DepthStencilRb);
		glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilRb);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)width, (GLsizei)height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glGenFramebuffers(1, &m_Fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_Fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorTexture, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthStencilRb);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			EQN_CORE_ERROR("OpenGL: scene framebuffer is incomplete ({0}x{1})", width, height);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLBackend::DestroySceneTarget()
	{
		if (m_Fbo) { glDeleteFramebuffers(1, &m_Fbo); m_Fbo = 0; }
		if (m_ColorTexture) { glDeleteTextures(1, &m_ColorTexture); m_ColorTexture = 0; }
		if (m_DepthStencilRb) { glDeleteRenderbuffers(1, &m_DepthStencilRb); m_DepthStencilRb = 0; }
	}

	void OpenGLBackend::DestroyObjects()
	{
		DestroySceneTarget();
		DestroyUniformBuffers();
		if (m_WhiteTexture) { glDeleteTextures(1, &m_WhiteTexture); m_WhiteTexture = 0; }
		if (m_Program) { glDeleteProgram(m_Program); m_Program = 0; }
	}

	void OpenGLBackend::SetSceneTargetSize(u32 width, u32 height)
	{
		if (width == 0 || height == 0) return;
		m_PendingWidth = width;
		m_PendingHeight = height;
		m_PendingResize = true;
	}

	// Frame
	bool OpenGLBackend::BeginFrame()
	{
		EQN_PROFILE_SCOPE("OpenGLBackend::BeginFrame");
		m_FrameStartTime = NowMs();
		m_Stats.drawCalls = 0;
		m_Stats.objectsTotal = 0;
		m_Stats.objectsVisible = 0;

		// The window buffer is cleared here (and not in EndFrame): the application
		// code and the editor UI can then draw into it during the frame, exactly
		// like the old App loop did with Renderer::Clear().
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		if (s_glClipControl) s_glClipControl(EQN_GL_LOWER_LEFT, EQN_GL_NEGATIVE_ONE_TO_ONE);
		glViewport(0, 0, (GLsizei)m_WindowWidth, (GLsizei)m_WindowHeight);
		glDisable(GL_SCISSOR_TEST);
		glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		m_FrameActive = true;
		return true;
	}

	void OpenGLBackend::EndFrame()
	{
		if (!m_FrameActive) return;
		EQN_PROFILE_SCOPE("OpenGLBackend::EndFrame");

		// The ImGui UI goes to the window (the scene lives in its own framebuffer)
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		if (s_glClipControl) s_glClipControl(EQN_GL_LOWER_LEFT, EQN_GL_NEGATIVE_ONE_TO_ONE);
		glViewport(0, 0, (GLsizei)m_WindowWidth, (GLsizei)m_WindowHeight);

		ImDrawData* drawData = ImGui::GetDrawData();
		if (m_ImGuiReady && drawData)
		{
			ImGui_ImplOpenGL3_RenderDrawData(drawData);

#ifdef IMGUI_HAS_VIEWPORT
			// Detached ImGui windows (multi-viewport): only meaningful on OpenGL
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				GLFWwindow* backup = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup);
			}
#endif
		}

		if (m_Window)
			glfwSwapBuffers((GLFWwindow*)m_Window);

		float cpuMs = (float)(NowMs() - m_FrameStartTime);
		m_Stats.cpuFrameMs = m_Stats.cpuFrameMs * 0.9f + cpuMs * 0.1f;

		m_FrameActive = false;
	}

	// Scene recording
	void OpenGLBackend::BeginScene(const SceneCamera& camera, const SceneLighting& lighting,
	                               const SceneEffects& effects, const Vec4& clearColor)
	{
		if (!m_FrameActive || !m_Program) return;

		if (m_PendingResize && (m_PendingWidth != m_TargetWidth || m_PendingHeight != m_TargetHeight))
		{
			CreateSceneTarget(m_PendingWidth, m_PendingHeight);
			m_PendingResize = false;
		}

		// ---- per-frame uniforms (binding 0) ----
		SceneUniform ubo{};
		ubo.viewProj = camera.viewProj;
		ubo.cameraPos = Vec4(camera.position, 1.0f);

		Vec3 toLight = -lighting.direction;
		const float len = glm::length(toLight);
		toLight = len > 0.0001f ? toLight / len : Vec3(0.0f, 1.0f, 0.0f);
		ubo.lightDir = Vec4(toLight, 0.0f);
		ubo.lightColor = Vec4(lighting.color, 1.0f);
		ubo.params = Vec4(lighting.ambient, 0.0f, 0.0f, effects.time);

		// ---- post-process parameters (applied at the end of mesh.frag) ----
		ubo.post0 = Vec4(effects.exposure, effects.contrast, effects.saturation, (float)effects.toneMapping);
		ubo.post1 = Vec4(effects.vignetteAmount, effects.vignetteHardness, effects.grainAmount, effects.aberrationOffset);
		ubo.post2 = Vec4(effects.shadowBalance, effects.enabled ? 1.0f : 0.0f);
		ubo.post3 = Vec4(effects.midtoneBalance, 0.0f);
		ubo.post4 = Vec4(effects.highlightBalance, 0.0f);
		ubo.viewport = Vec4((float)m_TargetWidth, (float)m_TargetHeight, 0.0f, 0.0f);

		glBindBuffer(GL_UNIFORM_BUFFER, m_SceneUbo);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SceneUniform), &ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_SceneUbo);

		// ---- per-frame state ----
		m_ObjectSlot = 0;
		m_BoundTexture = 0;
		m_BoundMesh = nullptr;

		glBindFramebuffer(GL_FRAMEBUFFER, m_Fbo);

		// THE key point for having the same image as Vulkan:
		//   UPPER_LEFT    -> the Y axis of the window goes down, like Vulkan
		//   ZERO_TO_ONE   -> the depth range is 0..1, like Vulkan (and like GLM_FORCE_DEPTH_ZERO_TO_ONE)
		if (s_glClipControl) s_glClipControl(EQN_GL_UPPER_LEFT, EQN_GL_ZERO_TO_ONE);

		glViewport(0, 0, (GLsizei)m_TargetWidth, (GLsizei)m_TargetHeight);
		glDisable(GL_SCISSOR_TEST);

		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);

		glUseProgram(m_Program);

		// texture unit 1 = binding 1 of "uniform sampler2D baseTexture"
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, m_WhiteTexture);
		m_BoundTexture = m_WhiteTexture;
	}

	void OpenGLBackend::DrawItem(const Equinox::Gfx::DrawItem& item)
	{
		if (!m_FrameActive || !m_Program || !item.mesh) return;

		if (m_ObjectSlot >= MaxObjectsPerFrame)
		{
			m_Stats.objectsTotal++;
			return;
		}

		// ---- per-object data, selected with a dynamic offset (binding 2) ----
		const u32 slot = m_ObjectSlot++;
		const GLintptr offset = (GLintptr)slot * (GLintptr)m_ObjectStride;

		ObjectUniform object{};
		object.model = item.model;
		object.color = item.color;
		object.params = item.params;
		object.lightDir = item.lightDir;
		object.lightColor = item.lightColor;
		object.emissive = item.emissive;

		glBindBuffer(GL_UNIFORM_BUFFER, m_ObjectUbo);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(ObjectUniform), &object);
		glBindBufferRange(GL_UNIFORM_BUFFER, 2, m_ObjectUbo, offset, sizeof(ObjectUniform));

		// ---- texture (unit 1) ----
		GLObject texture = item.texture ? (GLObject)item.texture->GetRendererID() : m_WhiteTexture;
		if (!texture) texture = m_WhiteTexture;
		if (texture != m_BoundTexture)
		{
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, texture);
			m_BoundTexture = texture;
		}

		// ---- geometry (the engine mesh owns its VAO in OpenGL) ----
		item.mesh->Bind();
		item.mesh->Draw();

		m_Stats.drawCalls++;
		m_Stats.objectsVisible++;
		m_Stats.objectsTotal++;
	}

	void OpenGLBackend::EndScene()
	{
		if (!m_FrameActive) return;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(0);

		// Back to the classic OpenGL conventions for ImGui / the application code
		if (s_glClipControl) s_glClipControl(EQN_GL_LOWER_LEFT, EQN_GL_NEGATIVE_ONE_TO_ONE);
		glViewport(0, 0, (GLsizei)m_WindowWidth, (GLsizei)m_WindowHeight);
	}

	u64 OpenGLBackend::GetSceneTextureId()
	{
		// For the OpenGL ImGui backend an "ImTextureID" is simply the GL texture name.
		return (u64)m_ColorTexture;
	}

	// ImGui
	bool OpenGLBackend::ImGuiInit(void* windowHandle)
	{
		if (!ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)windowHandle, true))
		{
			EQN_CORE_ERROR("ImGui_ImplGlfw_InitForOpenGL failed");
			return false;
		}
		if (!ImGui_ImplOpenGL3_Init("#version 460"))
		{
			EQN_CORE_ERROR("ImGui_ImplOpenGL3_Init failed");
			return false;
		}
		m_ImGuiReady = true;
		return true;
	}

	void OpenGLBackend::ImGuiShutdown()
	{
		if (!m_ImGuiReady) return;
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		m_ImGuiReady = false;
	}

	void OpenGLBackend::ImGuiNewFrame()
	{
		if (!m_ImGuiReady) return;

		// Standard order: platform backend (GLFW) first, then the renderer backend.
		ImGui_ImplGlfw_NewFrame();
		ImGui_ImplOpenGL3_NewFrame();
	}

	void OpenGLBackend::ImGuiRenderDrawData()
	{
		// The real drawing happens in EndFrame() (same order as the Vulkan backend,
		// where the ImGui draw data is recorded into the frame command buffer).
	}
}

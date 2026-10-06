#pragma once

#include "equinox/graphics/GfxBackend.h"

#include <string>
#include <vector>

namespace Equinox::Gfx
{
	class OpenGLBackend : public IBackend
	{
	public:
		using GLObject = unsigned int;   // GLuint / GLenum

		static constexpr u32 MaxObjectsPerFrame = 8192;

		OpenGLBackend() = default;
		virtual ~OpenGLBackend() override = default;

		// ---- identity ----------------------------------------------------
		BackendType GetType() const override { return BackendType::OpenGL; }
		const char* GetName() const override { return "OpenGL"; }
		const GPUInfo& GetGPUInfo() const override { return m_GPUInfo; }

		// ---- life cycle --------------------------------------------------
		void Init(void* windowHandle, u32 width, u32 height, bool vsync) override;
		void Shutdown() override;
		void OnWindowResize(u32 width, u32 height) override;
		void SetVSync(bool vsync) override;

		// ---- frame -------------------------------------------------------
		bool BeginFrame() override;
		void EndFrame() override;
		bool IsFrameActive() const override { return m_FrameActive; }

		// ---- scene -------------------------------------------------------
		void SetSceneTargetSize(u32 width, u32 height) override;
		SceneTargetSize GetSceneTargetSize() const override { return { m_TargetWidth, m_TargetHeight }; }

		void BeginScene(const SceneCamera& camera, const SceneLighting& lighting,
		                const SceneEffects& effects, const Vec4& clearColor) override;
		void DrawItem(const Equinox::Gfx::DrawItem& item) override;
		void EndScene() override;

		u64 GetSceneTextureId() override;

		// ---- ImGui -------------------------------------------------------
		bool ImGuiInit(void* windowHandle) override;
		void ImGuiShutdown() override;
		void ImGuiNewFrame() override;
		void ImGuiRenderDrawData() override;
		bool IsImGuiReady() const override { return m_ImGuiReady; }

		// ---- present surface (the default framebuffer of the window) ------
		u32 GetSwapchainImageCount() const override { return 1; }
		u64 GetSwapchainFormat() const override { return 0x8058u; }   // GL_RGBA8 (same as VK_FORMAT_R8G8B8A8_UNORM)
		SceneTargetSize GetSwapchainExtent() const override { return { m_WindowWidth, m_WindowHeight }; }

		FrameStats& GetStats() override { return m_Stats; }

	private:
		// ---- setup ----
		void LoadExtraFunctions();          // glShaderBinary / glSpecializeShader / glClipControl
		bool CreateShaderProgram();         // SPIR-V first, GLSL source as a fallback
		void CreateSceneTarget(u32 width, u32 height);
		void DestroySceneTarget();
		void CreateUniformBuffers();
		void DestroyUniformBuffers();
		void CreateWhiteTexture();
		void DestroyObjects();

		GLObject CompileSpirVStage(const std::string& spvPath, u32 glStage, const char* debugName);
		GLObject CompileGLSLStage(const std::string& sourcePath, u32 glStage, const char* debugName);

		// ---- state ----
		void* m_Window = nullptr;
		u32 m_WindowWidth = 1280;
		u32 m_WindowHeight = 720;
		bool m_FrameActive = false;
		bool m_ImGuiReady = false;
		bool m_SpirVSupported = false;
		double m_FrameStartTime = 0.0;

		// ---- scene render target ----
		GLObject m_Fbo = 0;
		GLObject m_ColorTexture = 0;
		GLObject m_DepthStencilRb = 0;
		u32 m_TargetWidth = 1280;
		u32 m_TargetHeight = 720;
		u32 m_PendingWidth = 1280;
		u32 m_PendingHeight = 720;
		bool m_PendingResize = false;

		// ---- pipeline / uniforms ----
		GLObject m_Program = 0;
		GLObject m_SceneUbo = 0;
		GLObject m_ObjectUbo = 0;
		GLObject m_WhiteTexture = 0;
		u32 m_ObjectStride = kObjectUniformAlignment;
		u32 m_ObjectSlot = 0;

		GLObject m_BoundTexture = 0;
		const void* m_BoundMesh = nullptr;

		GPUInfo m_GPUInfo;
		FrameStats m_Stats;
	};
}

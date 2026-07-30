#include "eqnpch.h"
#include "equinox/renderer/openGL/GLRendererAPI.h"
#include "equinox/core/Log.h"

#include <glad/glad.h>

#define EQN_GL_CHECK_ERROR() CheckError(__FILE__, __LINE__)

namespace Equinox
{
	void GLRendererAPI::Init()
	{
		// Glad should already be initialized by Window class
		// Verify GLAD loaded properly
		if (!gladLoadGL()) {
			EQN_CORE_CRITICAL("Failed to initialize Glad!");
			return;
		}

		EnableDepthTest(true);
		EnableBlending(true);
		SetBlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SetClearColor({ 0.15, 0.15, 0.15, 1.0 });
		//SetClearColor({ 1.00, 0.95, 0.97, 1.0 });

		EQN_CORE_INFO("OpenGL Renderer initialized");
		EQN_CORE_TRACE(" - Vendor: {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
		EQN_CORE_TRACE(" - Renderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
		EQN_CORE_TRACE(" - Version: {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	}

	void GLRendererAPI::Shutdown()
	{
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		m_Meshes.clear();
	}

	void GLRendererAPI::BindFramebuffer(const std::shared_ptr<Framebuffer>& framebuffer)
	{
		if (framebuffer)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->GetRendererID());
			Renderer::SetViewport(0, 0, framebuffer->GetWidth(), framebuffer->GetHeight());
		}
		else 
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	void GLRendererAPI::SetViewport(u32 x, u32 y, u32 width, u32 height)
	{
		glViewport(x, y, width, height);
		EQN_GL_CHECK_ERROR();
	}

	void GLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		m_ClearColor = color;
		glClearColor(color.r, color.g, color.b, color.a);
		EQN_GL_CHECK_ERROR();
	}

	void GLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		EQN_GL_CHECK_ERROR();
	}

	void GLRendererAPI::EnableDepthTest(bool enable)
	{
		m_DepthTestEnabled = enable;
		enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
		EQN_GL_CHECK_ERROR();
	}

	void GLRendererAPI::EnableBlending(bool enable)
	{
		m_BlendingEnabled = enable;
		enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
		EQN_GL_CHECK_ERROR();
	}

	void GLRendererAPI::SetBlendFunction(u32 srcFactor, u32 dstFactor)
	{
		glBlendFunc(srcFactor, dstFactor);
		EQN_GL_CHECK_ERROR();
	}

	void GLRendererAPI::SubmitMesh(const std::shared_ptr<Mesh>& mesh)
	{
		auto glMesh = std::dynamic_pointer_cast<GLMesh>(mesh);
		if (!glMesh)
		{
			EQN_CORE_WARN("GLRendererAPI::SubmitMesh - Invalid mesh type submitted!");
			return;
		}

		m_Meshes.push_back(glMesh);
	}

	void GLRendererAPI::DrawIndexed(u32 count)
	{
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
		EQN_GL_CHECK_ERROR();
	}

	void GLRendererAPI::DrawFrame()
	{
		for (auto mesh : m_Meshes)
		{
			mesh->Bind(); EQN_GL_CHECK_ERROR();
			mesh->Draw(); EQN_GL_CHECK_ERROR();
		}
	}

	void GLRendererAPI::CheckError(const char* file, int line)
	{
		while (GLenum error = glGetError())
		{
			std::string errorStr;
			switch (error)
			{
				case GL_INVALID_ENUM:      errorStr = "INVALID_ENUM";      break;
				case GL_INVALID_VALUE:     errorStr = "INVALID_VALUE";     break;
				case GL_INVALID_OPERATION: errorStr = "INVALID_OPERATION"; break;
				case GL_OUT_OF_MEMORY:     errorStr = "OUT_OF_MEMORY";     break;
				default:                   errorStr = "UNKNOWN_ERROR";     break;
			}
			EQN_CORE_ERROR("OpenGL Error ({0}) at {1}:{2}", errorStr, file, line);
		}
	}
}
#include "eqnpch.h"
#include "equinox/graphics/GfxRenderer.h"
#include "equinox/graphics/GfxBackend.h"
#include "equinox/graphics/GfxScene.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/core/Log.h"

namespace Equinox::Gfx
{
	void GfxRenderer::Init(void* windowHandle, u32 width, u32 height, bool vsync)
	{
		if (Backend::Exists())
		{
			EQN_CORE_WARN("GfxRenderer::Init called twice: ignored");
			return;
		}

		const RendererAPI::API api = RendererAPI::GetAPI();

		BackendType type = BackendType::OpenGL;
		if (api == RendererAPI::API::Vulkan)
			type = BackendType::Vulkan;

		if (type == BackendType::OpenGL)
			Renderer::Init(RendererAPI::API::OpenGL, windowHandle);

		if (!Backend::Create(type, windowHandle, width, height, vsync))
		{
			EQN_CORE_CRITICAL("Could not initialize the {0} backend", ToString(type));
			return;
		}

		GfxScene::Init();
		GfxScene::SetTargetSize(width, height);
	}

	void GfxRenderer::Shutdown()
	{
		const bool wasOpenGL = Backend::Exists() && Backend::Get().GetType() == BackendType::OpenGL;

		GfxScene::Shutdown();
		Backend::Destroy();

		if (wasOpenGL)
			Renderer::Shutdown();
	}

	void GfxRenderer::OnWindowResize(u32 width, u32 height)
	{
		if (Backend::Exists()) Backend::Get().OnWindowResize(width, height);
	}

	void GfxRenderer::SetVSync(bool vsync)
	{
		if (Backend::Exists()) Backend::Get().SetVSync(vsync);
	}

	bool GfxRenderer::BeginFrame()
	{
		if (!Backend::Exists()) return false;
		return Backend::Get().BeginFrame();
	}

	void GfxRenderer::EndFrame()
	{
		if (Backend::Exists()) Backend::Get().EndFrame();
	}

	bool GfxRenderer::IsFrameActive()
	{
		return Backend::Exists() && Backend::Get().IsFrameActive();
	}

	void GfxRenderer::SetSceneTargetSize(u32 width, u32 height)
	{
		if (Backend::Exists()) Backend::Get().SetSceneTargetSize(width, height);
	}

	SceneTargetSize GfxRenderer::GetSceneTargetSize()
	{
		if (!Backend::Exists()) return {};
		return Backend::Get().GetSceneTargetSize();
	}

	void GfxRenderer::BeginScene(const SceneCamera& camera, const SceneLighting& lighting,
		const SceneEffects& effects, const Vec4& clearColor)
	{
		if (Backend::Exists()) Backend::Get().BeginScene(camera, lighting, effects, clearColor);
	}

	void GfxRenderer::DrawItem(const Equinox::Gfx::DrawItem& item)
	{
		if (Backend::Exists()) Backend::Get().DrawItem(item);
	}

	void GfxRenderer::EndScene()
	{
		if (Backend::Exists()) Backend::Get().EndScene();
	}

	u64 GfxRenderer::GetSceneTextureId()
	{
		if (!Backend::Exists()) return 0;
		return Backend::Get().GetSceneTextureId();
	}

	bool GfxRenderer::ImGuiInit(void* windowHandle)
	{
		if (!Backend::Exists()) return false;
		return Backend::Get().ImGuiInit(windowHandle);
	}

	void GfxRenderer::ImGuiShutdown()
	{
		if (Backend::Exists()) Backend::Get().ImGuiShutdown();
	}

	void GfxRenderer::ImGuiNewFrame()
	{
		if (Backend::Exists()) Backend::Get().ImGuiNewFrame();
	}

	void GfxRenderer::ImGuiRenderDrawData()
	{
		if (Backend::Exists()) Backend::Get().ImGuiRenderDrawData();
	}

	bool GfxRenderer::IsImGuiReady()
	{
		return Backend::Exists() && Backend::Get().IsImGuiReady();
	}

	FrameStats& GfxRenderer::GetStats()
	{
		static FrameStats s_Empty;
		if (!Backend::Exists()) return s_Empty;
		return Backend::Get().GetStats();
	}

	const GPUInfo& GfxRenderer::GetGPUInfo()
	{
		static GPUInfo s_Empty;
		if (!Backend::Exists()) return s_Empty;
		return Backend::Get().GetGPUInfo();
	}

	const char* GfxRenderer::GetBackendName()
	{
		if (!Backend::Exists()) return "None";
		return Backend::Get().GetName();
	}

	BackendType GfxRenderer::GetBackendType()
	{
		if (!Backend::Exists()) return BackendType::OpenGL;
		return Backend::Get().GetType();
	}

	bool GfxRenderer::IsBackendReady()
	{
		return Backend::Exists();
	}

	u32 GfxRenderer::GetSwapchainImageCount()
	{
		if (!Backend::Exists()) return 0;
		return Backend::Get().GetSwapchainImageCount();
	}

	u64 GfxRenderer::GetSwapchainFormat()
	{
		if (!Backend::Exists()) return 0;
		return Backend::Get().GetSwapchainFormat();
	}

	SceneTargetSize GfxRenderer::GetSwapchainExtent()
	{
		if (!Backend::Exists()) return {};
		return Backend::Get().GetSwapchainExtent();
	}
}
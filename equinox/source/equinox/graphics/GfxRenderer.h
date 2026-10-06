#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/graphics/GfxBackend.h"

namespace Equinox::Gfx
{
	class GfxRenderer
	{
	public:
		static constexpr u32 MaxFramesInFlight = 2;

		static void Init(void* windowHandle, u32 width, u32 height, bool vsync);
		static void Shutdown();

		static void OnWindowResize(u32 width, u32 height);
		static void SetVSync(bool vsync);

		static bool BeginFrame();
		static void EndFrame();
		static bool IsFrameActive();

		static void SetSceneTargetSize(u32 width, u32 height);
		static SceneTargetSize GetSceneTargetSize();

		static void BeginScene(const SceneCamera& camera, const SceneLighting& lighting,
			const SceneEffects& effects,
			const Vec4& clearColor = Vec4(0.08f, 0.08f, 0.08f, 1.0f));
		static void DrawItem(const DrawItem& item);
		static void EndScene();

		static u64 GetSceneTextureId();

		static bool ImGuiInit(void* windowHandle);
		static void ImGuiShutdown();
		static void ImGuiNewFrame();
		static void ImGuiRenderDrawData();
		static bool IsImGuiReady();

		static FrameStats& GetStats();
		static const GPUInfo& GetGPUInfo();
		static const char* GetBackendName();
		static BackendType GetBackendType();
		static bool IsBackendReady();

		static u32 GetSwapchainImageCount();
		static u64 GetSwapchainFormat();
		static SceneTargetSize GetSwapchainExtent();
	};
}
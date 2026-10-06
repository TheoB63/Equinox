#pragma once

#include "equinox/graphics/GfxBackend.h"
#include "equinox/graphics/GfxContext.h"
#include "equinox/graphics/GfxSwapchain.h"
#include "equinox/graphics/vulkan/VulkanDeferredRenderer.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Equinox::Gfx
{
	// Owns Vulkan's frame/swapchain life cycle. Scene rendering itself lives in
	// VulkanDeferredRenderer so that swapchain code, render techniques and ImGui
	// do not accumulate in one monolithic backend.
	class VulkanBackend : public IBackend
	{
	public:
		VulkanBackend();
		~VulkanBackend() override;

		BackendType GetType() const override { return BackendType::Vulkan; }
		const char* GetName() const override { return "Vulkan 1.3 Deferred"; }
		const GPUInfo& GetGPUInfo() const override { return m_GPUInfo; }

		void Init(void* windowHandle, u32 width, u32 height, bool vsync) override;
		void Shutdown() override;
		void OnWindowResize(u32 width, u32 height) override;
		void SetVSync(bool vsync) override;

		bool BeginFrame() override;
		void EndFrame() override;
		bool IsFrameActive() const override { return m_FrameActive; }

		void SetSceneTargetSize(u32 width, u32 height) override;
		SceneTargetSize GetSceneTargetSize() const override;

		void BeginScene(const SceneCamera& camera, const SceneLighting& lighting,
			const SceneEffects& effects, const Vec4& clearColor) override;
		void DrawItem(const Equinox::Gfx::DrawItem& item) override;
		void EndScene() override;

		u64 GetSceneTextureId() override;

		bool ImGuiInit(void* windowHandle) override;
		void ImGuiShutdown() override;
		void ImGuiNewFrame() override;
		void ImGuiRenderDrawData() override {}
		bool IsImGuiReady() const override;

		u32 GetSwapchainImageCount() const override { return m_Swapchain ? m_Swapchain->GetImageCount() : 0; }
		u64 GetSwapchainFormat() const override { return m_Swapchain ? (u64)m_Swapchain->GetFormat() : 0; }
		SceneTargetSize GetSwapchainExtent() const override;

		VkCommandBuffer GetCommandBuffer() const { return m_Frames[m_FrameIndex].cmd; }
		u32 GetFrameIndex() const { return m_FrameIndex; }
		FrameStats& GetStats() override { return m_Stats; }

	private:
		struct FrameData
		{
			VkCommandPool pool = VK_NULL_HANDLE;
			VkCommandBuffer cmd = VK_NULL_HANDLE;
			VkSemaphore imageAvailable = VK_NULL_HANDLE;
			VkFence inFlight = VK_NULL_HANDLE;
			VkQueryPool queries = VK_NULL_HANDLE;
			bool queriesWritten = false;
		};

		static constexpr u32 MaxFramesInFlight = 2;

		void CreateFrameResources();
		void DestroyFrameResources();
		void CreateRenderFinishedSemaphores();
		void DestroyRenderFinishedSemaphores();
		void RecreateSwapchain();
		void InvalidateSceneTexture();

		void* m_Window = nullptr;
		std::unique_ptr<GfxSwapchain> m_Swapchain;
		std::unique_ptr<VulkanDeferredRenderer> m_SceneRenderer;

		FrameData m_Frames[MaxFramesInFlight];
		std::vector<VkSemaphore> m_RenderFinished;

		u32 m_FrameIndex = 0;
		u32 m_ImageIndex = 0;
		bool m_FrameActive = false;
		bool m_NeedsRecreate = false;
		bool m_ImGuiReady = false;
		double m_FrameStartTime = 0.0;
		u64 m_SceneTextureId = 0;

		GPUInfo m_GPUInfo;
		FrameStats m_Stats;
	};
}

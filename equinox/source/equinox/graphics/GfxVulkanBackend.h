#pragma once

#include "equinox/graphics/GfxBackend.h"
#include "equinox/graphics/GfxContext.h"
#include "equinox/graphics/GfxResources.h"
#include "equinox/graphics/GfxSwapchain.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Equinox::Gfx
{
	class GfxPipeline;

	class VulkanBackend : public IBackend
	{
	public:
		VulkanBackend() = default;
		virtual ~VulkanBackend() override = default;

		// ---- identity ----------------------------------------------------
		BackendType GetType() const override { return BackendType::Vulkan; }
		const char* GetName() const override { return "Vulkan"; }
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
		void ImGuiRenderDrawData() override {}
		bool IsImGuiReady() const override;

		// ---- present surface (GfxSwapchain) ------------------------------
		u32 GetSwapchainImageCount() const override { return m_Swapchain ? m_Swapchain->GetImageCount() : 0; }
		u64 GetSwapchainFormat() const override { return m_Swapchain ? (u64)m_Swapchain->GetFormat() : 0; }
		SceneTargetSize GetSwapchainExtent() const override
		{
			if (!m_Swapchain) return {};
			VkExtent2D e = m_Swapchain->GetExtent();
			return { e.width, e.height };
		}

		// ---- used internally by the frame loop / ImGui -------------------
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

		// GPU counterpart of one engine mesh
		struct GpuMesh
		{
			GfxBuffer vertices;
			GfxBuffer indices;
			u32 indexCount = 0;
			u32 vertexStride = 0;
		};

		// GPU counterpart of one engine texture
		struct GpuTexture
		{
			GfxImage image;
			VkSampler sampler = VK_NULL_HANDLE;
			bool valid = false;
		};

		static constexpr u32 MaxFramesInFlight = 2;
		static constexpr u32 MaxObjectsPerFrame = 8192;

		// ---- setup helpers ----
		void CreateFrameResources();
		void DestroyFrameResources();
		void CreateRenderFinishedSemaphores();
		void DestroyRenderFinishedSemaphores();
		void RecreateSwapchain();
		void CreateSceneTargets(u32 width, u32 height);
		void DestroySceneTargets();
		void ApplyPendingTargetResize();
		void CreateScenePipeline();
		void DestroyScenePipeline();
		VkSampler CreateSampler(VkFilter filter, VkSamplerAddressMode address, float maxAnisotropy, float maxLod);

		// ---- resource caches ----
		GpuMesh* GetOrCreateMesh(const Mesh* mesh);
		GpuTexture* GetOrCreateTexture(const Texture* texture);
		GpuTexture* GetWhiteTexture();
		void DestroyResourceCaches();

		// ---- frame state ----
		void* m_Window = nullptr;
		std::unique_ptr<GfxSwapchain> m_Swapchain;
		FrameData m_Frames[MaxFramesInFlight];
		std::vector<VkSemaphore> m_RenderFinished;

		u32 m_FrameIndex = 0;
		u32 m_ImageIndex = 0;
		bool m_FrameActive = false;
		bool m_NeedsRecreate = false;
		bool m_ImGuiReady = false;
		double m_FrameStartTime = 0.0;

		// ---- scene render target ----
		GfxImage m_Color;
		GfxImage m_Depth;
		VkSampler m_TargetSampler = VK_NULL_HANDLE;
		u64 m_SceneTextureId = 0;
		u32 m_TargetWidth = 1280;
		u32 m_TargetHeight = 720;
		u32 m_PendingWidth = 1280;
		u32 m_PendingHeight = 720;

		// ---- scene pipeline / descriptors ----
		std::unique_ptr<GfxPipeline> m_Pipeline;
		VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_Pool = VK_NULL_HANDLE;
		VkDescriptorSet m_Sets[MaxFramesInFlight] = {};

		GfxBuffer m_SceneUbo[MaxFramesInFlight];
		GfxBuffer m_ObjectUbo;
		u32 m_ObjectSlot = 0;

		VkSampler m_TextureSampler = VK_NULL_HANDLE;

		// ---- caches ----
		std::unordered_map<const Mesh*, GpuMesh> m_MeshCache;
		std::unordered_map<const Texture*, GpuTexture> m_TextureCache;
		GpuTexture m_WhiteTexture;
		VkSampler m_GuiSampler = VK_NULL_HANDLE;

		// ---- per-frame bind state (avoid useless rebinds) ----
		const GpuMesh* m_BoundMesh = nullptr;
		const GpuTexture* m_BoundTexture = nullptr;

		GPUInfo m_GPUInfo;
		FrameStats m_Stats;
	};
}
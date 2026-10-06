#include "eqnpch.h"
#include "equinox/graphics/GfxVulkanBackend.h"
#include "equinox/graphics/GfxImGui.h"
#include "equinox/graphics/GfxResources.h"
#include "equinox/graphics/vulkan/VulkanDeferredRenderer.h"
#include "equinox/core/Log.h"
#include "equinox/core/Profiler.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <cstdio>

namespace Equinox::Gfx
{
	namespace
	{
		double NowMs()
		{
			using namespace std::chrono;
			return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
		}
	}

	VulkanBackend::VulkanBackend() = default;
	VulkanBackend::~VulkanBackend() = default;

	void VulkanBackend::Init(void* windowHandle, u32 width, u32 height, bool vsync)
	{
		m_Window = windowHandle;
		GfxContext::Init(windowHandle);
		m_Swapchain = std::make_unique<GfxSwapchain>(width, height, vsync);

		auto& context = GfxContext::Get();
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(context.GetPhysicalDevice(), &properties);

		char apiVersion[64]{};
		std::snprintf(apiVersion, sizeof(apiVersion), "Vulkan %u.%u.%u",
			VK_VERSION_MAJOR(properties.apiVersion),
			VK_VERSION_MINOR(properties.apiVersion),
			VK_VERSION_PATCH(properties.apiVersion));

		char driver[64]{};
		std::snprintf(driver, sizeof(driver), "driver %u.%u.%u",
			VK_VERSION_MAJOR(properties.driverVersion),
			VK_VERSION_MINOR(properties.driverVersion),
			VK_VERSION_PATCH(properties.driverVersion));

		m_GPUInfo.device = context.GetDeviceName();
		m_GPUInfo.apiVersion = apiVersion;
		m_GPUInfo.driver = driver;
		m_GPUInfo.maxAnisotropy = std::max(context.GetMaxAnisotropy(), 1.0f);
		m_GPUInfo.timestamps = context.SupportsTimestamps();
		m_GPUInfo.multiViewport = false;

		CreateFrameResources();

		m_SceneRenderer = std::make_unique<VulkanDeferredRenderer>();
		m_SceneRenderer->Init(width, height);

		EQN_CORE_INFO("Vulkan 1.3 backend ready: deferred PBR + SSAO + transparency + bloom ({0} swapchain images)",
			m_Swapchain->GetImageCount());
	}

	void VulkanBackend::Shutdown()
	{
		if (!m_Swapchain) return;

		VkDevice device = GfxContext::Get().GetDevice();
		vkDeviceWaitIdle(device);

		InvalidateSceneTexture();
		if (m_SceneRenderer)
		{
			m_SceneRenderer->Shutdown();
			m_SceneRenderer.reset();
		}

		DestroyFrameResources();
		m_Swapchain.reset();
		GfxContext::Shutdown();
	}

	void VulkanBackend::CreateFrameResources()
	{
		auto& context = GfxContext::Get();
		VkDevice device = context.GetDevice();

		for (u32 i = 0; i < MaxFramesInFlight; ++i)
		{
			FrameData& frame = m_Frames[i];

			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = context.GetGraphicsQueue().familyIndex;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			vkCreateCommandPool(device, &poolInfo, nullptr, &frame.pool);

			VkCommandBufferAllocateInfo allocateInfo{};
			allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocateInfo.commandPool = frame.pool;
			allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocateInfo.commandBufferCount = 1;
			vkAllocateCommandBuffers(device, &allocateInfo, &frame.cmd);

			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.imageAvailable);

			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			vkCreateFence(device, &fenceInfo, nullptr, &frame.inFlight);

			if (context.SupportsTimestamps())
			{
				VkQueryPoolCreateInfo queryInfo{};
				queryInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
				queryInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
				queryInfo.queryCount = 2;
				vkCreateQueryPool(device, &queryInfo, nullptr, &frame.queries);
			}
		}

		CreateRenderFinishedSemaphores();
	}

	void VulkanBackend::DestroyFrameResources()
	{
		VkDevice device = GfxContext::Get().GetDevice();
		DestroyRenderFinishedSemaphores();

		for (FrameData& frame : m_Frames)
		{
			if (frame.queries) vkDestroyQueryPool(device, frame.queries, nullptr);
			if (frame.inFlight) vkDestroyFence(device, frame.inFlight, nullptr);
			if (frame.imageAvailable) vkDestroySemaphore(device, frame.imageAvailable, nullptr);
			if (frame.pool) vkDestroyCommandPool(device, frame.pool, nullptr);
			frame = {};
		}
	}

	void VulkanBackend::CreateRenderFinishedSemaphores()
	{
		VkDevice device = GfxContext::Get().GetDevice();
		m_RenderFinished.resize(m_Swapchain->GetImageCount());

		VkSemaphoreCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for (VkSemaphore& semaphore : m_RenderFinished)
			vkCreateSemaphore(device, &info, nullptr, &semaphore);
	}

	void VulkanBackend::DestroyRenderFinishedSemaphores()
	{
		if (!GfxContext::Get().GetDevice()) return;
		VkDevice device = GfxContext::Get().GetDevice();
		for (VkSemaphore semaphore : m_RenderFinished)
			if (semaphore) vkDestroySemaphore(device, semaphore, nullptr);
		m_RenderFinished.clear();
	}

	void VulkanBackend::OnWindowResize(u32 width, u32 height)
	{
		(void)width;
		(void)height;
		m_NeedsRecreate = true;
	}

	void VulkanBackend::SetVSync(bool vsync)
	{
		if (!m_Swapchain) return;
		vkDeviceWaitIdle(GfxContext::Get().GetDevice());
		DestroyRenderFinishedSemaphores();
		m_Swapchain->SetVSync(vsync);
		CreateRenderFinishedSemaphores();
	}

	void VulkanBackend::RecreateSwapchain()
	{
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize((GLFWwindow*)m_Window, &width, &height);

		vkDeviceWaitIdle(GfxContext::Get().GetDevice());
		DestroyRenderFinishedSemaphores();
		m_Swapchain->Resize((u32)width, (u32)height);
		if (m_Swapchain->IsValid()) CreateRenderFinishedSemaphores();
		m_NeedsRecreate = false;
	}

	bool VulkanBackend::BeginFrame()
	{
		EQN_PROFILE_SCOPE("VulkanBackend::BeginFrame");
		m_FrameActive = false;

		if (m_NeedsRecreate || !m_Swapchain->IsValid())
		{
			RecreateSwapchain();
			if (!m_Swapchain->IsValid()) return false;
		}

		VkDevice device = GfxContext::Get().GetDevice();
		FrameData& frame = m_Frames[m_FrameIndex];
		vkWaitForFences(device, 1, &frame.inFlight, VK_TRUE, UINT64_MAX);

		if (frame.queries && frame.queriesWritten)
		{
			u64 timestamps[2]{};
			VkResult result = vkGetQueryPoolResults(device, frame.queries, 0, 2,
				sizeof(timestamps), timestamps, sizeof(u64), VK_QUERY_RESULT_64_BIT);
			if (result == VK_SUCCESS && timestamps[1] >= timestamps[0])
			{
				const double ns = double(timestamps[1] - timestamps[0]) * GfxContext::Get().GetTimestampPeriod();
				const float ms = (float)(ns * 1e-6);
				m_Stats.gpuFrameMs = m_Stats.gpuFrameMs * 0.9f + ms * 0.1f;
			}
		}

		VkResult acquire = m_Swapchain->AcquireNextImage(frame.imageAvailable, m_ImageIndex);
		if (acquire == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_NeedsRecreate = true;
			return false;
		}
		if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR)
		{
			EQN_CORE_ERROR("vkAcquireNextImageKHR failed ({0})", (int)acquire);
			return false;
		}
		if (acquire == VK_SUBOPTIMAL_KHR) m_NeedsRecreate = true;

		vkResetFences(device, 1, &frame.inFlight);
		vkResetCommandPool(device, frame.pool, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(frame.cmd, &beginInfo);

		if (frame.queries)
		{
			vkCmdResetQueryPool(frame.cmd, frame.queries, 0, 2);
			vkCmdWriteTimestamp2(frame.cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frame.queries, 0);
		}

		m_FrameStartTime = NowMs();
		m_Stats.drawCalls = 0;
		m_Stats.geometryDrawCalls = 0;
		m_Stats.transparentDrawCalls = 0;
		m_Stats.fullscreenPasses = 0;
		m_Stats.opaqueObjects = 0;
		m_Stats.transparentObjects = 0;
		m_Stats.bloomPasses = 0;
		m_FrameActive = true;
		return true;
	}

	void VulkanBackend::EndFrame()
	{
		if (!m_FrameActive) return;
		EQN_PROFILE_SCOPE("VulkanBackend::EndFrame");

		FrameData& frame = m_Frames[m_FrameIndex];
		VkCommandBuffer cmd = frame.cmd;
		VkImage swapImage = m_Swapchain->GetImage(m_ImageIndex);
		VkExtent2D extent = m_Swapchain->GetExtent();

		TransitionRawImage(cmd, swapImage, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderingAttachmentInfo color{};
		color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		color.imageView = m_Swapchain->GetImageView(m_ImageIndex);
		color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color.clearValue.color = { { 0.08f, 0.08f, 0.08f, 1.0f } };

		VkRenderingInfo rendering{};
		rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		rendering.renderArea = { { 0, 0 }, extent };
		rendering.layerCount = 1;
		rendering.colorAttachmentCount = 1;
		rendering.pColorAttachments = &color;

		vkCmdBeginRendering(cmd, &rendering);
		GfxImGui::Record(cmd);
		vkCmdEndRendering(cmd);

		TransitionRawImage(cmd, swapImage, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		if (frame.queries)
		{
			vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frame.queries, 1);
			frame.queriesWritten = true;
		}
		vkEndCommandBuffer(cmd);

		VkSemaphoreSubmitInfo waitInfo{};
		waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitInfo.semaphore = frame.imageAvailable;
		waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkCommandBufferSubmitInfo commandInfo{};
		commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		commandInfo.commandBuffer = cmd;

		VkSemaphoreSubmitInfo signalInfo{};
		signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalInfo.semaphore = m_RenderFinished[m_ImageIndex];
		signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

		VkSubmitInfo2 submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submit.waitSemaphoreInfoCount = 1;
		submit.pWaitSemaphoreInfos = &waitInfo;
		submit.commandBufferInfoCount = 1;
		submit.pCommandBufferInfos = &commandInfo;
		submit.signalSemaphoreInfoCount = 1;
		submit.pSignalSemaphoreInfos = &signalInfo;

		auto& context = GfxContext::Get();
		{
			std::lock_guard<std::mutex> lock(context.GetQueueMutex());
			VkResult result = vkQueueSubmit2(context.GetGraphicsQueue().handle, 1, &submit, frame.inFlight);
			if (result != VK_SUCCESS) EQN_CORE_ERROR("vkQueueSubmit2 failed ({0})", (int)result);
		}

		VkResult present = m_Swapchain->Present(m_RenderFinished[m_ImageIndex], m_ImageIndex);
		if (present == VK_ERROR_OUT_OF_DATE_KHR || present == VK_SUBOPTIMAL_KHR)
			m_NeedsRecreate = true;
		else if (present != VK_SUCCESS)
			EQN_CORE_ERROR("vkQueuePresentKHR failed ({0})", (int)present);

		const float cpuMs = (float)(NowMs() - m_FrameStartTime);
		m_Stats.cpuFrameMs = m_Stats.cpuFrameMs * 0.9f + cpuMs * 0.1f;
		m_FrameIndex = (m_FrameIndex + 1) % MaxFramesInFlight;
		m_FrameActive = false;
	}

	void VulkanBackend::SetSceneTargetSize(u32 width, u32 height)
	{
		if (!m_SceneRenderer || width == 0 || height == 0) return;
		const SceneTargetSize current = m_SceneRenderer->GetTargetSize();
		if (current.width == width && current.height == height) return;
		InvalidateSceneTexture();
		m_SceneRenderer->SetTargetSize(width, height);
	}

	SceneTargetSize VulkanBackend::GetSceneTargetSize() const
	{
		return m_SceneRenderer ? m_SceneRenderer->GetTargetSize() : SceneTargetSize{};
	}

	void VulkanBackend::BeginScene(const SceneCamera& camera, const SceneLighting& lighting,
		const SceneEffects& effects, const Vec4& clearColor)
	{
		if (m_FrameActive && m_SceneRenderer)
			m_SceneRenderer->BeginScene(m_FrameIndex, camera, lighting, effects, clearColor);
	}

	void VulkanBackend::DrawItem(const Equinox::Gfx::DrawItem& item)
	{
		if (m_FrameActive && m_SceneRenderer)
			m_SceneRenderer->SubmitItem(item);
	}

	void VulkanBackend::EndScene()
	{
		if (m_FrameActive && m_SceneRenderer)
			m_SceneRenderer->EndScene(m_Frames[m_FrameIndex].cmd, m_FrameIndex, m_Stats);
	}

	void VulkanBackend::InvalidateSceneTexture()
	{
		if (m_SceneTextureId)
		{
			GfxImGui::RemoveTexture(m_SceneTextureId);
			m_SceneTextureId = 0;
		}
	}

	u64 VulkanBackend::GetSceneTextureId()
	{
		if (!m_SceneRenderer || !GfxImGui::IsReady()) return 0;
		if (!m_SceneTextureId && m_SceneRenderer->GetFinalView())
			m_SceneTextureId = GfxImGui::AddTexture(m_SceneRenderer->GetFinalSampler(), m_SceneRenderer->GetFinalView());
		return m_SceneTextureId;
	}

	bool VulkanBackend::ImGuiInit(void* windowHandle)
	{
		if (!ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)windowHandle, true))
		{
			EQN_CORE_ERROR("ImGui_ImplGlfw_InitForVulkan failed");
			return false;
		}
		GfxImGui::Init();
		m_ImGuiReady = GfxImGui::IsReady();
		return m_ImGuiReady;
	}

	void VulkanBackend::ImGuiShutdown()
	{
		if (!m_ImGuiReady) return;
		InvalidateSceneTexture();
		GfxImGui::Shutdown();
		ImGui_ImplGlfw_Shutdown();
		m_ImGuiReady = false;
	}

	void VulkanBackend::ImGuiNewFrame()
	{
		ImGui_ImplGlfw_NewFrame();
		GfxImGui::NewFrame();
	}

	bool VulkanBackend::IsImGuiReady() const
	{
		return m_ImGuiReady && GfxImGui::IsReady();
	}

	SceneTargetSize VulkanBackend::GetSwapchainExtent() const
	{
		if (!m_Swapchain) return {};
		const VkExtent2D extent = m_Swapchain->GetExtent();
		return { extent.width, extent.height };
	}
}

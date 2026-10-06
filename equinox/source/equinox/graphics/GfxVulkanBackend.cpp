#include "eqnpch.h"
#include "equinox/graphics/GfxVulkanBackend.h"
#include "equinox/graphics/GfxPipeline.h"
#include "equinox/graphics/GfxShader.h"
#include "equinox/graphics/GfxImGui.h"
#include "equinox/core/Log.h"
#include "equinox/core/Profiler.h"
#include "equinox/renderer/Mesh.h"
#include "equinox/renderer/Model.h"
#include "equinox/renderer/Texture.h"
#include "equinox/renderer/null/NullResources.h"
#include "equinox/utils/ImageUtils.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#include <GLFW/glfw3.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace Equinox::Gfx
{
	static double NowMs()
	{
		using namespace std::chrono;
		return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
	}

	static constexpr u32 kVertexStride = sizeof(Vertex);

	void VulkanBackend::Init(void* windowHandle, u32 width, u32 height, bool vsync)
	{
		m_Window = windowHandle;

		GfxContext::Init(windowHandle);
		m_Swapchain = std::make_unique<GfxSwapchain>(width, height, vsync);

		auto& ctx = GfxContext::Get();

		// ---- informations shown by the "GPU" panel ----
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(ctx.GetPhysicalDevice(), &props);

		char version[64] = {};
		std::snprintf(version, sizeof(version), "Vulkan %u.%u.%u",
			VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));

		char driver[64] = {};
		std::snprintf(driver, sizeof(driver), "driver %u.%u.%u",
			VK_VERSION_MAJOR(props.driverVersion), VK_VERSION_MINOR(props.driverVersion), VK_VERSION_PATCH(props.driverVersion));

		m_GPUInfo.device = ctx.GetDeviceName();
		m_GPUInfo.apiVersion = version;
		m_GPUInfo.driver = driver;
		m_GPUInfo.maxAnisotropy = ctx.GetMaxAnisotropy() > 1.0f ? ctx.GetMaxAnisotropy() : 1.0f;
		m_GPUInfo.timestamps = ctx.SupportsTimestamps();
		m_GPUInfo.multiViewport = false;   // one swapchain per window is not managed (yet) in Vulkan

		CreateFrameResources();

		// ---- scene render target (SceneColor + depth) ----
		m_PendingWidth = width;
		m_PendingHeight = height;
		m_TargetSampler = CreateSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.0f, 0.0f);
		CreateSceneTargets(width, height);
		m_PendingWidth = m_TargetWidth;
		m_PendingHeight = m_TargetHeight;

		// ---- texture sampler (shared by every engine texture) ----
		const float maxAniso = ctx.GetMaxAnisotropy();
		m_TextureSampler = CreateSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT,
			maxAniso > 8.0f ? 8.0f : maxAniso, VK_LOD_CLAMP_NONE);
		m_GuiSampler = CreateSampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.0f, VK_LOD_CLAMP_NONE);

		CreateScenePipeline();

		EQN_CORE_INFO("Vulkan backend ready ({0} swapchain images, {1} frames in flight)",
			m_Swapchain->GetImageCount(), MaxFramesInFlight);
	}

	void VulkanBackend::Shutdown()
	{
		if (!m_Swapchain) return;
		auto device = GfxContext::Get().GetDevice();
		vkDeviceWaitIdle(device);

		DestroyResourceCaches();
		DestroyScenePipeline();

		if (m_WhiteTexture.sampler) vkDestroySampler(device, m_WhiteTexture.sampler, nullptr);
		if (m_TextureSampler) vkDestroySampler(device, m_TextureSampler, nullptr);
		if (m_TargetSampler) vkDestroySampler(device, m_TargetSampler, nullptr);
		if (m_GuiSampler) vkDestroySampler(device, m_GuiSampler, nullptr);
		m_WhiteTexture = {};
		m_TextureSampler = VK_NULL_HANDLE;
		m_TargetSampler = VK_NULL_HANDLE;
		m_GuiSampler = VK_NULL_HANDLE;

		DestroySceneTargets();
		DestroyFrameResources();

		m_Swapchain.reset();
		GfxContext::Shutdown();
	}

	void VulkanBackend::CreateFrameResources()
	{
		auto& ctx = GfxContext::Get();
		auto device = ctx.GetDevice();

		for (u32 i = 0; i < MaxFramesInFlight; i++)
		{
			FrameData& f = m_Frames[i];

			// 1 command pool PER frame in flight: reset in one go at the start of the frame.
			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = ctx.GetGraphicsQueue().familyIndex;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			vkCreateCommandPool(device, &poolInfo, nullptr, &f.pool);

			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = f.pool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;
			vkAllocateCommandBuffers(device, &allocInfo, &f.cmd);

			VkSemaphoreCreateInfo semInfo{};
			semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			vkCreateSemaphore(device, &semInfo, nullptr, &f.imageAvailable);

			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;   // the 1st frame must not wait
			vkCreateFence(device, &fenceInfo, nullptr, &f.inFlight);

			if (ctx.SupportsTimestamps())
			{
				VkQueryPoolCreateInfo qInfo{};
				qInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
				qInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
				qInfo.queryCount = 2;
				vkCreateQueryPool(device, &qInfo, nullptr, &f.queries);
			}
		}

		CreateRenderFinishedSemaphores();
	}

	void VulkanBackend::DestroyFrameResources()
	{
		auto device = GfxContext::Get().GetDevice();

		for (auto& f : m_Frames)
		{
			if (f.queries) vkDestroyQueryPool(device, f.queries, nullptr);
			if (f.inFlight) vkDestroyFence(device, f.inFlight, nullptr);
			if (f.imageAvailable) vkDestroySemaphore(device, f.imageAvailable, nullptr);
			if (f.pool) vkDestroyCommandPool(device, f.pool, nullptr);   // also destroys its command buffer
			f = FrameData{};
		}
		DestroyRenderFinishedSemaphores();
	}

	void VulkanBackend::CreateRenderFinishedSemaphores()
	{
		auto device = GfxContext::Get().GetDevice();
		m_RenderFinished.resize(m_Swapchain->GetImageCount());
		VkSemaphoreCreateInfo semInfo{};
		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for (auto& s : m_RenderFinished)
			vkCreateSemaphore(device, &semInfo, nullptr, &s);
	}

	void VulkanBackend::DestroyRenderFinishedSemaphores()
	{
		auto device = GfxContext::Get().GetDevice();
		for (auto s : m_RenderFinished)
			vkDestroySemaphore(device, s, nullptr);
		m_RenderFinished.clear();
	}

	void VulkanBackend::OnWindowResize(u32 width, u32 height)
	{
		(void)width; (void)height;
		// We do not recreate here (we may be in the middle of a frame): it is done at the next BeginFrame
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
		int w = 0, h = 0;
		glfwGetFramebufferSize((GLFWwindow*)m_Window, &w, &h);

		vkDeviceWaitIdle(GfxContext::Get().GetDevice());
		DestroyRenderFinishedSemaphores();
		m_Swapchain->Resize((u32)w, (u32)h);   // if w or h == 0: invalid swapchain (minimized window)
		if (m_Swapchain->IsValid())
			CreateRenderFinishedSemaphores();
		m_NeedsRecreate = false;
	}

	void VulkanBackend::CreateSceneTargets(u32 width, u32 height)
	{
		m_TargetWidth = width;
		m_TargetHeight = height;

		m_Color = CreateImage2D(width, height, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT);

		VkImageAspectFlags depthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		VkFormat df = GfxContext::Get().GetDepthFormat();
		if (df == VK_FORMAT_D32_SFLOAT_S8_UINT || df == VK_FORMAT_D24_UNORM_S8_UINT)
			depthAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;   // the image carries both aspects; the view only uses depth
		m_Depth = CreateImage2D(width, height, df, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthAspect);
	}

	void VulkanBackend::DestroySceneTargets()
	{
		GfxImGui::RemoveTexture(m_SceneTextureId);
		m_SceneTextureId = 0;
		DestroyImage(m_Color);
		DestroyImage(m_Depth);
	}

	void VulkanBackend::ApplyPendingTargetResize()
	{
		if (m_PendingWidth == m_TargetWidth && m_PendingHeight == m_TargetHeight) return;
		if (m_PendingWidth == 0 || m_PendingHeight == 0) return;

		// Simple and safe: wait for the GPU to finish, then recreate the images.
		vkDeviceWaitIdle(GfxContext::Get().GetDevice());
		DestroySceneTargets();
		CreateSceneTargets(m_PendingWidth, m_PendingHeight);
	}

	void VulkanBackend::SetSceneTargetSize(u32 width, u32 height)
	{
		if (width == 0 || height == 0) return;
		m_PendingWidth = width;
		m_PendingHeight = height;
	}

	VkSampler VulkanBackend::CreateSampler(VkFilter filter, VkSamplerAddressMode address, float maxAnisotropy, float maxLod)
	{
		VkSamplerCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.magFilter = filter;
		info.minFilter = filter;
		info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		info.addressModeU = address;
		info.addressModeV = address;
		info.addressModeW = address;
		info.anisotropyEnable = maxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE;
		info.maxAnisotropy = maxAnisotropy > 1.0f ? maxAnisotropy : 1.0f;
		info.minLod = 0.0f;
		info.maxLod = maxLod;
		VkSampler sampler = VK_NULL_HANDLE;
		vkCreateSampler(GfxContext::Get().GetDevice(), &info, nullptr, &sampler);
		return sampler;
	}

	void VulkanBackend::CreateScenePipeline()
	{
		auto& ctx = GfxContext::Get();
		auto device = ctx.GetDevice();

		// ---- layout: binding 0 = scene UBO, 1 = texture, 2 = object UBO (dynamic) ----
		VkDescriptorSetLayoutBinding bindings[3]{};
		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[2].binding = 2;
		bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		bindings[2].descriptorCount = 1;
		bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 3;
		layoutInfo.pBindings = bindings;
		vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_SetLayout);

		VkDescriptorPoolSize poolSizes[3]{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = MaxFramesInFlight;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = MaxFramesInFlight;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		poolSizes[2].descriptorCount = MaxFramesInFlight;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = MaxFramesInFlight;
		poolInfo.poolSizeCount = 3;
		poolInfo.pPoolSizes = poolSizes;
		vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_Pool);

		VkDescriptorSetLayout layouts[MaxFramesInFlight];
		for (auto& l : layouts) l = m_SetLayout;
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_Pool;
		allocInfo.descriptorSetCount = MaxFramesInFlight;
		allocInfo.pSetLayouts = layouts;
		vkAllocateDescriptorSets(device, &allocInfo, m_Sets);

		// ---- buffers: one scene UBO per frame + one big object UBO ----
		m_ObjectUbo = CreateBuffer(
			VkDeviceSize(MaxFramesInFlight) * MaxObjectsPerFrame * kObjectUniformAlignment,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);

		GpuTexture* white = GetWhiteTexture();
		if (!white) white = &m_WhiteTexture;

		for (u32 f = 0; f < MaxFramesInFlight; f++)
		{
			m_SceneUbo[f] = CreateBuffer(sizeof(SceneUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);

			VkDescriptorBufferInfo sceneInfo{ m_SceneUbo[f].buffer, 0, sizeof(SceneUniform) };
			VkDescriptorImageInfo imageInfo{ white->sampler, white->image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
			VkDescriptorBufferInfo objectInfo{ m_ObjectUbo.buffer, 0, sizeof(ObjectUniform) };

			VkWriteDescriptorSet writes[3]{};
			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].dstSet = m_Sets[f];
			writes[0].dstBinding = 0;
			writes[0].descriptorCount = 1;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[0].pBufferInfo = &sceneInfo;
			writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].dstSet = m_Sets[f];
			writes[1].dstBinding = 1;
			writes[1].descriptorCount = 1;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[1].pImageInfo = &imageInfo;
			writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[2].dstSet = m_Sets[f];
			writes[2].dstBinding = 2;
			writes[2].descriptorCount = 1;
			writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			writes[2].pBufferInfo = &objectInfo;
			vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);
		}

		// ---- pipeline: same shaders as OpenGL (SPIR-V), no push constant ----
		auto vert = std::make_shared<GfxShader>(GfxShader::ResolveShaderPath("mesh.vert.spv"), ShaderStage::Vertex);
		auto frag = std::make_shared<GfxShader>(GfxShader::ResolveShaderPath("mesh.frag.spv"), ShaderStage::Fragment);

		PipelineConfig config;
		config.colorFormats = { VK_FORMAT_R8G8B8A8_UNORM };
		config.depthFormat = ctx.GetDepthFormat();
		config.depthTest = true;
		config.depthWrite = true;
		config.blend = false;
		config.cullMode = VK_CULL_MODE_BACK_BIT;
		config.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;   // (the scene uses a flipped viewport, see BeginScene)
		config.vertexBindings = { { 0, kVertexStride, VK_VERTEX_INPUT_RATE_VERTEX } };
		config.vertexAttributes = {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, (u32)offsetof(Vertex, Position)  },
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, (u32)offsetof(Vertex, Normal)    },
			{ 2, 0, VK_FORMAT_R32G32_SFLOAT,    (u32)offsetof(Vertex, TexCoord0) },
			{ 3, 0, VK_FORMAT_R32G32_SFLOAT,    (u32)offsetof(Vertex, TexCoord1) },
			{ 4, 0, VK_FORMAT_R32G32B32_SFLOAT, (u32)offsetof(Vertex, Tangent)   },
		};
		config.pushConstants = {};   // <- nothing: per-object data is the dynamic UBO (binding 2)
		m_Pipeline = std::make_unique<GfxPipeline>(config, vert, frag, m_SetLayout);
	}

	void VulkanBackend::DestroyScenePipeline()
	{
		auto device = GfxContext::Get().GetDevice();

		m_Pipeline.reset();

		for (auto& b : m_SceneUbo) DestroyBuffer(b);
		DestroyBuffer(m_ObjectUbo);

		if (m_Pool) vkDestroyDescriptorPool(device, m_Pool, nullptr);
		if (m_SetLayout) vkDestroyDescriptorSetLayout(device, m_SetLayout, nullptr);
		m_Pool = VK_NULL_HANDLE;
		m_SetLayout = VK_NULL_HANDLE;
	}

	VulkanBackend::GpuMesh* VulkanBackend::GetOrCreateMesh(const Mesh* mesh)
	{
		if (!mesh) return nullptr;

		auto it = m_MeshCache.find(mesh);
		if (it != m_MeshCache.end())
			return &it->second;

		// Under Vulkan the resource factories build CPU-only meshes
		// (renderer/null/NullResources.h) that keep the vertex/index data:
		// this is where they become real GPU buffers.
		const NullMesh* nullMesh = dynamic_cast<const NullMesh*>(mesh);
		if (!nullMesh)
		{
			EQN_CORE_ERROR("VulkanBackend: mesh is not a CPU mesh, cannot upload it");
			return nullptr;
		}

		auto vb = nullMesh->GetVertexBuffer();
		auto ib = nullMesh->GetIndexBuffer();
		if (!vb || !ib)
		{
			EQN_CORE_ERROR("VulkanBackend: mesh without vertex/index buffer");
			return nullptr;
		}

		const auto* nullVB = dynamic_cast<const NullVertexBuffer*>(vb.get());
		const auto* nullIB = dynamic_cast<const NullIndexBuffer*>(ib.get());
		if (!nullVB || !nullIB || nullVB->GetData().empty() || nullIB->GetIndices().empty())
			return nullptr;

		const u32 stride = nullVB->GetLayout().GetStride();
		if (stride != kVertexStride)
		{
			EQN_CORE_WARN("VulkanBackend: mesh vertex stride {0} != expected {1} (mesh.vert layout)", stride, kVertexStride);
			return nullptr;
		}

		GpuMesh gpu;
		gpu.vertices = CreateBufferWithData(nullVB->GetData().data(), nullVB->GetData().size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		gpu.indices = CreateBufferWithData(nullIB->GetIndices().data(), nullIB->GetIndices().size() * sizeof(u32), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		gpu.indexCount = (u32)nullIB->GetIndices().size();
		gpu.vertexStride = stride;

		auto [inserted, _] = m_MeshCache.emplace(mesh, gpu);
		return &inserted->second;
	}

	VulkanBackend::GpuTexture* VulkanBackend::GetWhiteTexture()
	{
		if (m_WhiteTexture.valid) return &m_WhiteTexture;

		const u8 pixel[4] = { 255, 255, 255, 255 };
		m_WhiteTexture.image = CreateTextureRGBA8(pixel, 1, 1, false);
		m_WhiteTexture.sampler = m_GuiSampler ? m_GuiSampler : CreateSampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.0f, VK_LOD_CLAMP_NONE);
		m_WhiteTexture.valid = true;
		return &m_WhiteTexture;
	}

	VulkanBackend::GpuTexture* VulkanBackend::GetOrCreateTexture(const Texture* texture)
	{
		if (!texture) return GetWhiteTexture();

		auto it = m_TextureCache.find(texture);
		if (it != m_TextureCache.end())
			return it->second.valid ? &it->second : GetWhiteTexture();

		// ---- 1) size known by the engine (NullTexture, from the image header) ----
		std::vector<u8> pixels;
		u32 width = 0, height = 0;

		if (const NullTexture* nullTexture = dynamic_cast<const NullTexture*>(texture))
		{
			width = nullTexture->GetWidth();
			height = nullTexture->GetHeight();
		}

		// ---- 2) otherwise decode the file (engine textures are sRGB images) ----
		if (pixels.empty())
		{
			const fs::path& path = texture->GetPath();
			if (path.empty() || !fs::exists(path))
			{
				EQN_CORE_WARN("VulkanBackend: texture '{0}' has no file to decode, using the white texture", path.string());
				m_TextureCache[texture] = GpuTexture{};
				return GetWhiteTexture();
			}

			int w = 0, h = 0, channels = 0;
			stbi_uc* data = stbi_load(path.string().c_str(), &w, &h, &channels, 4);   // force RGBA8
			if (!data)
			{
				EQN_CORE_ERROR("VulkanBackend: failed to load texture '{0}'", path.string());
				m_TextureCache[texture] = GpuTexture{};
				return GetWhiteTexture();
			}

			width = (u32)w;
			height = (u32)h;
			pixels.assign(data, data + (size_t)w * h * 4);
			stbi_image_free(data);
		}

		GpuTexture gpu;
		gpu.image = CreateTextureRGBA8(pixels.data(), width, height, true);
		gpu.sampler = m_TextureSampler;
		gpu.valid = gpu.image.image != VK_NULL_HANDLE;

		m_TextureCache[texture] = gpu;
		return gpu.valid ? &m_TextureCache[texture] : GetWhiteTexture();
	}

	void VulkanBackend::DestroyResourceCaches()
	{
		auto device = GfxContext::Get().GetDevice();

		for (auto& [mesh, gpu] : m_MeshCache)
		{
			DestroyBuffer(gpu.vertices);
			DestroyBuffer(gpu.indices);
		}
		m_MeshCache.clear();

		for (auto& [texture, gpu] : m_TextureCache)
		{
			DestroyImage(gpu.image);
			if (gpu.sampler && gpu.sampler != m_TextureSampler && gpu.sampler != m_GuiSampler)
				vkDestroySampler(device, gpu.sampler, nullptr);
		}
		m_TextureCache.clear();

		DestroyImage(m_WhiteTexture.image);
		if (m_WhiteTexture.sampler && m_WhiteTexture.sampler != m_TextureSampler && m_WhiteTexture.sampler != m_GuiSampler)
			vkDestroySampler(device, m_WhiteTexture.sampler, nullptr);
		m_WhiteTexture = {};
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

		auto device = GfxContext::Get().GetDevice();
		FrameData& f = m_Frames[m_FrameIndex];

		// 1) Wait until the GPU has finished the frame that used this slot
		vkWaitForFences(device, 1, &f.inFlight, VK_TRUE, UINT64_MAX);

		// 2) Read the GPU time of the frame that just finished
		if (f.queries && f.queriesWritten)
		{
			u64 ts[2] = { 0, 0 };
			VkResult r = vkGetQueryPoolResults(device, f.queries, 0, 2, sizeof(ts), ts, sizeof(u64), VK_QUERY_RESULT_64_BIT);
			if (r == VK_SUCCESS && ts[1] >= ts[0])
			{
				double ns = double(ts[1] - ts[0]) * GfxContext::Get().GetTimestampPeriod();
				float ms = (float)(ns * 1e-6);
				m_Stats.gpuFrameMs = m_Stats.gpuFrameMs * 0.9f + ms * 0.1f;
			}
		}

		// 3) Ask for the next image to display
		VkResult res = m_Swapchain->AcquireNextImage(f.imageAvailable, m_ImageIndex);
		if (res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_NeedsRecreate = true;
			return false;
		}
		if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
		{
			EQN_CORE_ERROR("vkAcquireNextImageKHR failed ({0})", (int)res);
			return false;
		}
		if (res == VK_SUBOPTIMAL_KHR) m_NeedsRecreate = true;

		// Only now (acquire succeeded) do we reset the fence: otherwise risk of an infinite wait
		vkResetFences(device, 1, &f.inFlight);

		// 4) Start again from an empty command buffer
		vkResetCommandPool(device, f.pool, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(f.cmd, &beginInfo);

		if (f.queries)
		{
			vkCmdResetQueryPool(f.cmd, f.queries, 0, 2);
			vkCmdWriteTimestamp2(f.cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, f.queries, 0);
		}

		m_FrameStartTime = NowMs();
		m_Stats.drawCalls = 0;
		m_FrameActive = true;
		return true;
	}

	void VulkanBackend::EndFrame()
	{
		if (!m_FrameActive) return;
		EQN_PROFILE_SCOPE("VulkanBackend::EndFrame");

		FrameData& f = m_Frames[m_FrameIndex];
		VkCommandBuffer cmd = f.cmd;
		VkImage swapImage = m_Swapchain->GetImage(m_ImageIndex);
		VkExtent2D extent = m_Swapchain->GetExtent();
		VkImageLayout swapLayout = VK_IMAGE_LAYOUT_UNDEFINED;   // freshly acquired image: undefined content

		// ---- the ImGui editor is drawn into the window (the scene is shown in it as a texture) ----
		TransitionRawImage(cmd, swapImage, VK_IMAGE_ASPECT_COLOR_BIT, swapLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		swapLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

		// The image must be in PRESENT_SRC to be displayed
		TransitionRawImage(cmd, swapImage, VK_IMAGE_ASPECT_COLOR_BIT, swapLayout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		if (f.queries)
		{
			vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, f.queries, 1);
			f.queriesWritten = true;
		}

		vkEndCommandBuffer(cmd);

		// ---- Submit to the GPU: waits for the image (imageAvailable), signals the end (renderFinished) ----
		VkSemaphoreSubmitInfo waitInfo{};
		waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitInfo.semaphore = f.imageAvailable;
		waitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

		VkCommandBufferSubmitInfo cmdInfo{};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		cmdInfo.commandBuffer = cmd;

		VkSemaphoreSubmitInfo signalInfo{};
		signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalInfo.semaphore = m_RenderFinished[m_ImageIndex];
		signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

		VkSubmitInfo2 submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submit.waitSemaphoreInfoCount = 1;
		submit.pWaitSemaphoreInfos = &waitInfo;
		submit.commandBufferInfoCount = 1;
		submit.pCommandBufferInfos = &cmdInfo;
		submit.signalSemaphoreInfoCount = 1;
		submit.pSignalSemaphoreInfos = &signalInfo;

		auto& ctx = GfxContext::Get();
		{
			std::lock_guard<std::mutex> lock(ctx.GetQueueMutex());
			VkResult r = vkQueueSubmit2(ctx.GetGraphicsQueue().handle, 1, &submit, f.inFlight);
			if (r != VK_SUCCESS) EQN_CORE_ERROR("vkQueueSubmit2 failed ({0})", (int)r);
		}

		VkResult present = m_Swapchain->Present(m_RenderFinished[m_ImageIndex], m_ImageIndex);
		if (present == VK_ERROR_OUT_OF_DATE_KHR || present == VK_SUBOPTIMAL_KHR)
			m_NeedsRecreate = true;
		else if (present != VK_SUCCESS)
			EQN_CORE_ERROR("vkQueuePresentKHR failed ({0})", (int)present);

		float cpuMs = (float)(NowMs() - m_FrameStartTime);
		m_Stats.cpuFrameMs = m_Stats.cpuFrameMs * 0.9f + cpuMs * 0.1f;

		m_FrameIndex = (m_FrameIndex + 1) % MaxFramesInFlight;
		m_FrameActive = false;
	}

	void VulkanBackend::BeginScene(const SceneCamera& camera, const SceneLighting& lighting,
		const SceneEffects& effects, const Vec4& clearColor)
	{
		if (!m_FrameActive) return;
		ApplyPendingTargetResize();

		const u32 frame = m_FrameIndex;
		VkCommandBuffer cmd = m_Frames[frame].cmd;

		// ---- per-frame uniforms ----
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

		std::memcpy(m_SceneUbo[frame].mapped, &ubo, sizeof(ubo));
		FlushBuffer(m_SceneUbo[frame]);

		// ---- per-frame draw state ----
		m_ObjectSlot = 0;
		m_BoundMesh = nullptr;
		m_BoundTexture = nullptr;

		// ---- render target -> attachments ----
		// We do not care about the previous content (we clear it): old layout = UNDEFINED.
		TransitionRawImage(cmd, m_Color.image, m_Color.aspect, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_Color.mipLevels);
		m_Color.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		TransitionRawImage(cmd, m_Depth.image, m_Depth.aspect, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, m_Depth.mipLevels);
		m_Depth.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

		VkRenderingAttachmentInfo colorAtt{};
		colorAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAtt.imageView = m_Color.view;
		colorAtt.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAtt.clearValue.color = { { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };

		VkRenderingAttachmentInfo depthAtt{};
		depthAtt.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAtt.imageView = m_Depth.view;
		depthAtt.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAtt.clearValue.depthStencil = { 1.0f, 0 };

		VkRenderingInfo rendering{};
		rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		rendering.renderArea = { { 0, 0 }, { m_TargetWidth, m_TargetHeight } };
		rendering.layerCount = 1;
		rendering.colorAttachmentCount = 1;
		rendering.pColorAttachments = &colorAtt;
		rendering.pDepthAttachment = &depthAtt;

		vkCmdBeginRendering(cmd, &rendering);

		// Viewport with a NEGATIVE height: flips the image vertically.
		// Vulkan has the Y axis pointing down, OpenGL up. This way the editor camera
		// (made for OpenGL) gives exactly the same image in both APIs, and the vertex
		// order (counter-clockwise) stays valid.
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = (float)m_TargetHeight;
		viewport.width = (float)m_TargetWidth;
		viewport.height = -(float)m_TargetHeight;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{ { 0, 0 }, { m_TargetWidth, m_TargetHeight } };
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		if (m_Pipeline && m_Pipeline->GetPipeline())
			m_Pipeline->Bind(cmd);
	}

	void VulkanBackend::DrawItem(const Equinox::Gfx::DrawItem& item)
	{
		if (!m_FrameActive || !m_Pipeline) return;

		GpuMesh* mesh = GetOrCreateMesh(item.mesh);
		if (!mesh) return;

		GpuTexture* texture = GetOrCreateTexture(item.texture);
		if (!texture) texture = GetWhiteTexture();
		if (!texture || !texture->valid) return;

		if (m_ObjectSlot >= MaxObjectsPerFrame)
		{
			m_Stats.objectsTotal++;
			return;   // out of object slots for this frame (increase MaxObjectsPerFrame)
		}

		const u32 frame = m_FrameIndex;
		VkCommandBuffer cmd = m_Frames[frame].cmd;

		// ---- per-object data ----
		const u32 slot = m_ObjectSlot++;
		const VkDeviceSize frameBase = VkDeviceSize(frame) * MaxObjectsPerFrame * kObjectUniformAlignment;
		const VkDeviceSize dynamicOffset = frameBase + VkDeviceSize(slot) * kObjectUniformAlignment;

		ObjectUniform object{};
		object.model = item.model;
		object.color = item.color;
		object.params = item.params;
		object.lightDir = item.lightDir;
		object.lightColor = item.lightColor;
		object.emissive = item.emissive;

		std::memcpy((u8*)m_ObjectUbo.mapped + dynamicOffset, &object, sizeof(ObjectUniform));

		// ---- geometry ----
		if (mesh != m_BoundMesh)
		{
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &mesh->vertices.buffer, &offset);
			vkCmdBindIndexBuffer(cmd, mesh->indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			m_BoundMesh = mesh;
		}

		// ---- texture: only rewrite the descriptor when it changes ----
		if (texture != m_BoundTexture)
		{
			VkDescriptorImageInfo imageInfo{ texture->sampler, texture->image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = m_Sets[frame];
			write.dstBinding = 1;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write.pImageInfo = &imageInfo;
			vkUpdateDescriptorSets(GfxContext::Get().GetDevice(), 1, &write, 0, nullptr);
			m_BoundTexture = texture;
		}

		// ---- bind (the object block is selected by the dynamic offset) ----
		const u32 dynamicOffsetU32 = (u32)dynamicOffset;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetLayout(),
			0, 1, &m_Sets[frame], 1, &dynamicOffsetU32);

		vkCmdDrawIndexed(cmd, mesh->indexCount, 1, 0, 0, 0);

		m_Stats.drawCalls++;
		m_Stats.objectsVisible++;
		m_Stats.objectsTotal++;
	}

	void VulkanBackend::EndScene()
	{
		if (!m_FrameActive) return;

		const u32 frame = m_FrameIndex;
		VkCommandBuffer cmd = m_Frames[frame].cmd;

		vkCmdEndRendering(cmd);

		if (m_ObjectSlot > 0)
			FlushBuffer(m_ObjectUbo);

		TransitionImage(cmd, m_Color, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	u64 VulkanBackend::GetSceneTextureId()
	{
		if (m_SceneTextureId == 0 && GfxImGui::IsReady() && m_Color.view != VK_NULL_HANDLE)
			m_SceneTextureId = GfxImGui::AddTexture(m_TargetSampler, m_Color.view);

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
}
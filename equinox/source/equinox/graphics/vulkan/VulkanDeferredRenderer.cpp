#include "eqnpch.h"
#include "equinox/graphics/vulkan/VulkanDeferredRenderer.h"

#include "equinox/graphics/GfxContext.h"
#include "equinox/graphics/GfxPipeline.h"
#include "equinox/graphics/GfxShader.h"
#include "equinox/core/Log.h"
#include "equinox/core/Profiler.h"
#include "equinox/renderer/Mesh.h"
#include "equinox/renderer/Model.h"
#include "equinox/renderer/Texture.h"
#include "equinox/renderer/null/NullResources.h"
#include "equinox/utils/ImageUtils.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstddef>
#include <initializer_list>
#include <random>

namespace Equinox::Gfx
{
	namespace
	{
		constexpr VkFormat GPositionFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		constexpr VkFormat GNormalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		constexpr VkFormat GAlbedoFormat = VK_FORMAT_R8G8B8A8_UNORM;
		constexpr VkFormat GEmissiveFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		constexpr VkFormat SsaoFormat = VK_FORMAT_R8_UNORM;
		constexpr VkFormat HdrFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		constexpr VkFormat FinalFormat = VK_FORMAT_R8G8B8A8_UNORM;

		VkDescriptorSetLayoutBinding Binding(u32 binding, VkDescriptorType type, VkShaderStageFlags stages)
		{
			VkDescriptorSetLayoutBinding result{};
			result.binding = binding;
			result.descriptorType = type;
			result.descriptorCount = 1;
			result.stageFlags = stages;
			return result;
		}

		VkDescriptorSetLayout CreateLayout(std::initializer_list<VkDescriptorSetLayoutBinding> bindings)
		{
			VkDescriptorSetLayoutCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			info.bindingCount = (u32)bindings.size();
			info.pBindings = bindings.begin();

			VkDescriptorSetLayout layout = VK_NULL_HANDLE;
			if (vkCreateDescriptorSetLayout(GfxContext::Get().GetDevice(), &info, nullptr, &layout) != VK_SUCCESS)
				EQN_CORE_CRITICAL("VulkanDeferredRenderer: descriptor set layout creation failed");
			return layout;
		}

		void WriteBuffer(VkDescriptorSet set, u32 binding, VkDescriptorType type,
			VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
		{
			VkDescriptorBufferInfo bufferInfo{ buffer, offset, range };
			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = set;
			write.dstBinding = binding;
			write.descriptorCount = 1;
			write.descriptorType = type;
			write.pBufferInfo = &bufferInfo;
			vkUpdateDescriptorSets(GfxContext::Get().GetDevice(), 1, &write, 0, nullptr);
		}

		void WriteImage(VkDescriptorSet set, u32 binding, VkSampler sampler, VkImageView view)
		{
			VkDescriptorImageInfo imageInfo{ sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = set;
			write.dstBinding = binding;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write.pImageInfo = &imageInfo;
			vkUpdateDescriptorSets(GfxContext::Get().GetDevice(), 1, &write, 0, nullptr);
		}

		void SetViewportAndScissor(VkCommandBuffer cmd, u32 width, u32 height, bool flipY)
		{
			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = flipY ? (float)height : 0.0f;
			viewport.width = (float)width;
			viewport.height = flipY ? -(float)height : (float)height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(cmd, 0, 1, &viewport);

			VkRect2D scissor{ { 0, 0 }, { width, height } };
			vkCmdSetScissor(cmd, 0, 1, &scissor);
		}

		void BeginRendering(VkCommandBuffer cmd,
			const std::vector<GfxImage*>& colors,
			VkAttachmentLoadOp colorLoad,
			const Vec4& clearColor,
			GfxImage* depth = nullptr,
			VkAttachmentLoadOp depthLoad = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VkAttachmentStoreOp depthStore = VK_ATTACHMENT_STORE_OP_DONT_CARE)
		{
			std::vector<VkRenderingAttachmentInfo> colorAttachments(colors.size());
			for (size_t i = 0; i < colors.size(); ++i)
			{
				VkRenderingAttachmentInfo& attachment = colorAttachments[i];
				attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				attachment.imageView = colors[i]->view;
				attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachment.loadOp = colorLoad;
				attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attachment.clearValue.color = { { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
			}

			VkRenderingAttachmentInfo depthAttachment{};
			if (depth)
			{
				depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				depthAttachment.imageView = depth->view;
				depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				depthAttachment.loadOp = depthLoad;
				depthAttachment.storeOp = depthStore;
				depthAttachment.clearValue.depthStencil = { 1.0f, 0 };
			}

			VkRenderingInfo rendering{};
			rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			rendering.renderArea = { { 0, 0 }, colors.front()->extent };
			rendering.layerCount = 1;
			rendering.colorAttachmentCount = (u32)colorAttachments.size();
			rendering.pColorAttachments = colorAttachments.data();
			rendering.pDepthAttachment = depth ? &depthAttachment : nullptr;
			vkCmdBeginRendering(cmd, &rendering);
		}

		std::shared_ptr<GfxShader> LoadShader(const char* name, ShaderStage stage)
		{
			return std::make_shared<GfxShader>(GfxShader::ResolveShaderPath(name), stage);
		}
	}

	VulkanDeferredRenderer::VulkanDeferredRenderer() = default;

	VulkanDeferredRenderer::~VulkanDeferredRenderer()
	{
		if (m_Initialized) Shutdown();
	}

	void VulkanDeferredRenderer::Init(u32 width, u32 height)
	{
		m_Width = std::max(width, 1u);
		m_Height = std::max(height, 1u);
		m_PendingWidth = m_Width;
		m_PendingHeight = m_Height;

		CreateSamplers();
		CreateTargets(m_Width, m_Height);
		CreateDescriptorInfrastructure();
		CreateUniformBuffers();
		AllocateFixedDescriptorSets();
		UpdateFixedDescriptorSets();
		CreatePipelines();

		m_DrawItems.reserve(MaxObjectsPerFrame);
		m_OpaqueItems.reserve(MaxObjectsPerFrame);
		m_TransparentItems.reserve(MaxObjectsPerFrame / 4);
		m_Initialized = true;
	}

	void VulkanDeferredRenderer::Shutdown()
	{
		if (!m_Initialized) return;
		vkDeviceWaitIdle(GfxContext::Get().GetDevice());

		DestroyPipelines();
		DestroyDescriptorInfrastructure(); // frees every set before referenced resources
		DestroyResourceCaches();
		DestroyUniformBuffers();
		DestroyTargets();
		DestroySamplers();

		m_DrawItems.clear();
		m_OpaqueItems.clear();
		m_TransparentItems.clear();
		m_Initialized = false;
	}

	void VulkanDeferredRenderer::SetTargetSize(u32 width, u32 height)
	{
		if (width == 0 || height == 0) return;
		m_PendingWidth = width;
		m_PendingHeight = height;
	}

	void VulkanDeferredRenderer::ApplyPendingResize()
	{
		if (m_PendingWidth == m_Width && m_PendingHeight == m_Height) return;

		vkDeviceWaitIdle(GfxContext::Get().GetDevice());
		DestroyTargets();
		CreateTargets(m_PendingWidth, m_PendingHeight);
		UpdateFixedDescriptorSets();
	}

	void VulkanDeferredRenderer::CreateTargets(u32 width, u32 height)
	{
		m_Width = std::max(width, 1u);
		m_Height = std::max(height, 1u);
		const u32 halfWidth = std::max(m_Width / 2, 1u);
		const u32 halfHeight = std::max(m_Height / 2, 1u);
		const VkImageUsageFlags sampledColor = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		m_GPositionRoughness = CreateImage2D(m_Width, m_Height, GPositionFormat, sampledColor, VK_IMAGE_ASPECT_COLOR_BIT);
		m_GNormalMetallic = CreateImage2D(m_Width, m_Height, GNormalFormat, sampledColor, VK_IMAGE_ASPECT_COLOR_BIT);
		m_GAlbedoAlpha = CreateImage2D(m_Width, m_Height, GAlbedoFormat, sampledColor, VK_IMAGE_ASPECT_COLOR_BIT);
		m_GEmissive = CreateImage2D(m_Width, m_Height, GEmissiveFormat, sampledColor, VK_IMAGE_ASPECT_COLOR_BIT);

		VkFormat depthFormat = GfxContext::Get().GetDepthFormat();
		VkImageAspectFlags depthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || depthFormat == VK_FORMAT_D24_UNORM_S8_UINT)
			depthAspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
		m_Depth = CreateImage2D(m_Width, m_Height, depthFormat,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthAspect);

		m_SsaoRaw = CreateImage2D(m_Width, m_Height, SsaoFormat, sampledColor, VK_IMAGE_ASPECT_COLOR_BIT);
		m_SsaoBlur = CreateImage2D(m_Width, m_Height, SsaoFormat, sampledColor, VK_IMAGE_ASPECT_COLOR_BIT);
		m_HdrColor = CreateImage2D(m_Width, m_Height, HdrFormat, sampledColor, VK_IMAGE_ASPECT_COLOR_BIT);
		// OpenGL extracts bloom at full resolution, then downsamples on the first
		// ping-pong blur. Keep the same graph for equal numeric settings.
		m_BloomExtract = CreateImage2D(m_Width, m_Height, HdrFormat, sampledColor, VK_IMAGE_ASPECT_COLOR_BIT);
		m_BloomPing[0] = CreateImage2D(halfWidth, halfHeight, HdrFormat, sampledColor, VK_IMAGE_ASPECT_COLOR_BIT);
		m_BloomPing[1] = CreateImage2D(halfWidth, halfHeight, HdrFormat, sampledColor, VK_IMAGE_ASPECT_COLOR_BIT);
		m_FinalColor = CreateImage2D(m_Width, m_Height, FinalFormat,
			sampledColor | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void VulkanDeferredRenderer::DestroyTargets()
	{
		DestroyImage(m_GPositionRoughness);
		DestroyImage(m_GNormalMetallic);
		DestroyImage(m_GAlbedoAlpha);
		DestroyImage(m_GEmissive);
		DestroyImage(m_Depth);
		DestroyImage(m_SsaoRaw);
		DestroyImage(m_SsaoBlur);
		DestroyImage(m_HdrColor);
		DestroyImage(m_BloomExtract);
		DestroyImage(m_BloomPing[0]);
		DestroyImage(m_BloomPing[1]);
		DestroyImage(m_FinalColor);
	}

	void VulkanDeferredRenderer::CreateSamplers()
	{
		VkDevice device = GfxContext::Get().GetDevice();

		VkSamplerCreateInfo linear{};
		linear.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		linear.magFilter = VK_FILTER_LINEAR;
		linear.minFilter = VK_FILTER_LINEAR;
		linear.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		linear.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		linear.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		linear.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		linear.minLod = 0.0f;
		linear.maxLod = VK_LOD_CLAMP_NONE;
		vkCreateSampler(device, &linear, nullptr, &m_LinearClampSampler);

		VkSamplerCreateInfo texture = linear;
		texture.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		texture.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		texture.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		const float maxAnisotropy = GfxContext::Get().GetMaxAnisotropy();
		texture.anisotropyEnable = maxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE;
		texture.maxAnisotropy = std::max(1.0f, std::min(maxAnisotropy, 8.0f));
		vkCreateSampler(device, &texture, nullptr, &m_TextureSampler);
	}

	void VulkanDeferredRenderer::DestroySamplers()
	{
		VkDevice device = GfxContext::Get().GetDevice();
		if (m_TextureSampler) vkDestroySampler(device, m_TextureSampler, nullptr);
		if (m_LinearClampSampler) vkDestroySampler(device, m_LinearClampSampler, nullptr);
		m_TextureSampler = VK_NULL_HANDLE;
		m_LinearClampSampler = VK_NULL_HANDLE;
	}

	void VulkanDeferredRenderer::CreateDescriptorInfrastructure()
	{
		const VkShaderStageFlags allGraphics = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		m_FrameSetLayout = CreateLayout({
			Binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, allGraphics),
			Binding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, allGraphics)
		});
		m_TextureSetLayout = CreateLayout({ Binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) });
		m_SsaoSetLayout = CreateLayout({
			Binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		});
		m_SingleTextureSetLayout = CreateLayout({ Binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) });
		m_LightingSetLayout = CreateLayout({
			Binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		});
		m_BloomExtractSetLayout = CreateLayout({
			Binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		});
		m_CompositeSetLayout = CreateLayout({
			Binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			Binding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		});

		VkDescriptorPoolSize sizes[3]{};
		sizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 64 };
		sizes[1] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, FrameCount };
		sizes[2] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxTextures + 128 };

		VkDescriptorPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		info.maxSets = MaxTextures + 128;
		info.poolSizeCount = 3;
		info.pPoolSizes = sizes;
		if (vkCreateDescriptorPool(GfxContext::Get().GetDevice(), &info, nullptr, &m_DescriptorPool) != VK_SUCCESS)
			EQN_CORE_CRITICAL("VulkanDeferredRenderer: descriptor pool creation failed");
	}

	void VulkanDeferredRenderer::DestroyDescriptorInfrastructure()
	{
		VkDevice device = GfxContext::Get().GetDevice();
		if (m_DescriptorPool) vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		if (m_CompositeSetLayout) vkDestroyDescriptorSetLayout(device, m_CompositeSetLayout, nullptr);
		if (m_BloomExtractSetLayout) vkDestroyDescriptorSetLayout(device, m_BloomExtractSetLayout, nullptr);
		if (m_LightingSetLayout) vkDestroyDescriptorSetLayout(device, m_LightingSetLayout, nullptr);
		if (m_SingleTextureSetLayout) vkDestroyDescriptorSetLayout(device, m_SingleTextureSetLayout, nullptr);
		if (m_SsaoSetLayout) vkDestroyDescriptorSetLayout(device, m_SsaoSetLayout, nullptr);
		if (m_TextureSetLayout) vkDestroyDescriptorSetLayout(device, m_TextureSetLayout, nullptr);
		if (m_FrameSetLayout) vkDestroyDescriptorSetLayout(device, m_FrameSetLayout, nullptr);

		m_DescriptorPool = VK_NULL_HANDLE;
		m_CompositeSetLayout = VK_NULL_HANDLE;
		m_BloomExtractSetLayout = VK_NULL_HANDLE;
		m_LightingSetLayout = VK_NULL_HANDLE;
		m_SingleTextureSetLayout = VK_NULL_HANDLE;
		m_SsaoSetLayout = VK_NULL_HANDLE;
		m_TextureSetLayout = VK_NULL_HANDLE;
		m_FrameSetLayout = VK_NULL_HANDLE;
	}

	VkDescriptorSet VulkanDeferredRenderer::AllocateSet(VkDescriptorSetLayout layout)
	{
		VkDescriptorSetAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool = m_DescriptorPool;
		info.descriptorSetCount = 1;
		info.pSetLayouts = &layout;

		VkDescriptorSet set = VK_NULL_HANDLE;
		if (vkAllocateDescriptorSets(GfxContext::Get().GetDevice(), &info, &set) != VK_SUCCESS)
			EQN_CORE_ERROR("VulkanDeferredRenderer: descriptor set allocation failed (pool exhausted?)");
		return set;
	}

	VkDescriptorSet VulkanDeferredRenderer::AllocateTextureSet(VkImageView view)
	{
		VkDescriptorSet set = AllocateSet(m_TextureSetLayout);
		if (set) WriteImage(set, 0, m_TextureSampler, view);
		return set;
	}

	void VulkanDeferredRenderer::CreateUniformBuffers()
	{
		for (GfxBuffer& buffer : m_SceneUbo)
			buffer = CreateBuffer(sizeof(DeferredSceneUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);

		m_ObjectUbo = CreateBuffer(
			VkDeviceSize(FrameCount) * MaxObjectsPerFrame * kObjectUniformAlignment,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);

		m_SsaoKernelUbo = CreateBuffer(sizeof(SsaoKernelUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
		SsaoKernelUniform kernel{};
		std::mt19937 random(0xE901A0u);
		std::uniform_real_distribution<float> zeroOne(0.0f, 1.0f);
		std::uniform_real_distribution<float> signedOne(-1.0f, 1.0f);

		for (u32 i = 0; i < SsaoKernelSize; ++i)
		{
			Vec3 sample(signedOne(random), signedOne(random), zeroOne(random));
			const float length = glm::length(sample);
			sample = length > 0.0001f ? sample / length : Vec3(0.0f, 0.0f, 1.0f);
			sample *= zeroOne(random);
			const float t = (float)i / (float)SsaoKernelSize;
			const float scale = glm::mix(0.1f, 1.0f, t * t);
			kernel.samples[i] = Vec4(sample * scale, 0.0f);
		}
		std::memcpy(m_SsaoKernelUbo.mapped, &kernel, sizeof(kernel));
		FlushBuffer(m_SsaoKernelUbo);
	}

	void VulkanDeferredRenderer::DestroyUniformBuffers()
	{
		for (GfxBuffer& buffer : m_SceneUbo) DestroyBuffer(buffer);
		DestroyBuffer(m_ObjectUbo);
		DestroyBuffer(m_SsaoKernelUbo);
	}

	void VulkanDeferredRenderer::AllocateFixedDescriptorSets()
	{
		for (u32 frame = 0; frame < FrameCount; ++frame)
		{
			m_FrameSets[frame] = AllocateSet(m_FrameSetLayout);
			m_SsaoSets[frame] = AllocateSet(m_SsaoSetLayout);
			m_LightingSets[frame] = AllocateSet(m_LightingSetLayout);
			m_BloomExtractSets[frame] = AllocateSet(m_BloomExtractSetLayout);
			m_CompositeSets[frame][0] = AllocateSet(m_CompositeSetLayout);
			m_CompositeSets[frame][1] = AllocateSet(m_CompositeSetLayout);
		}
		m_SsaoBlurSet = AllocateSet(m_SingleTextureSetLayout);
		for (VkDescriptorSet& set : m_BloomBlurSets)
			set = AllocateSet(m_SingleTextureSetLayout);
	}

	void VulkanDeferredRenderer::UpdateFixedDescriptorSets()
	{
		for (u32 frame = 0; frame < FrameCount; ++frame)
		{
			WriteBuffer(m_FrameSets[frame], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				m_SceneUbo[frame].buffer, 0, sizeof(DeferredSceneUniform));
			WriteBuffer(m_FrameSets[frame], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
				m_ObjectUbo.buffer, 0, sizeof(ObjectUniform));

			WriteImage(m_SsaoSets[frame], 0, m_LinearClampSampler, m_GPositionRoughness.view);
			WriteImage(m_SsaoSets[frame], 1, m_LinearClampSampler, m_GNormalMetallic.view);
			WriteBuffer(m_SsaoSets[frame], 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				m_SceneUbo[frame].buffer, 0, sizeof(DeferredSceneUniform));
			WriteBuffer(m_SsaoSets[frame], 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				m_SsaoKernelUbo.buffer, 0, sizeof(SsaoKernelUniform));

			WriteImage(m_LightingSets[frame], 0, m_LinearClampSampler, m_GPositionRoughness.view);
			WriteImage(m_LightingSets[frame], 1, m_LinearClampSampler, m_GNormalMetallic.view);
			WriteImage(m_LightingSets[frame], 2, m_LinearClampSampler, m_GAlbedoAlpha.view);
			WriteImage(m_LightingSets[frame], 3, m_LinearClampSampler, m_GEmissive.view);
			WriteImage(m_LightingSets[frame], 4, m_LinearClampSampler, m_SsaoBlur.view);
			WriteBuffer(m_LightingSets[frame], 5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				m_SceneUbo[frame].buffer, 0, sizeof(DeferredSceneUniform));

			WriteImage(m_BloomExtractSets[frame], 0, m_LinearClampSampler, m_HdrColor.view);
			WriteBuffer(m_BloomExtractSets[frame], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				m_SceneUbo[frame].buffer, 0, sizeof(DeferredSceneUniform));

			for (u32 bloom = 0; bloom < 2; ++bloom)
			{
				WriteImage(m_CompositeSets[frame][bloom], 0, m_LinearClampSampler, m_HdrColor.view);
				WriteImage(m_CompositeSets[frame][bloom], 1, m_LinearClampSampler, m_BloomPing[bloom].view);
				WriteBuffer(m_CompositeSets[frame][bloom], 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					m_SceneUbo[frame].buffer, 0, sizeof(DeferredSceneUniform));
				WriteImage(m_CompositeSets[frame][bloom], 3, m_LinearClampSampler, m_GPositionRoughness.view);
				WriteImage(m_CompositeSets[frame][bloom], 4, m_LinearClampSampler, m_GNormalMetallic.view);
				WriteImage(m_CompositeSets[frame][bloom], 5, m_LinearClampSampler, m_GAlbedoAlpha.view);
				WriteImage(m_CompositeSets[frame][bloom], 6, m_LinearClampSampler, m_GEmissive.view);
				WriteImage(m_CompositeSets[frame][bloom], 7, m_LinearClampSampler, m_SsaoBlur.view);
			}
		}

		WriteImage(m_SsaoBlurSet, 0, m_LinearClampSampler, m_SsaoRaw.view);
		WriteImage(m_BloomBlurSets[0], 0, m_LinearClampSampler, m_BloomExtract.view);
		WriteImage(m_BloomBlurSets[1], 0, m_LinearClampSampler, m_BloomPing[0].view);
		WriteImage(m_BloomBlurSets[2], 0, m_LinearClampSampler, m_BloomPing[1].view);
	}

	void VulkanDeferredRenderer::CreatePipelines()
	{
		const u32 stride = sizeof(Vertex);
		const std::vector<VkVertexInputBindingDescription> vertexBindings = {
			{ 0, stride, VK_VERTEX_INPUT_RATE_VERTEX }
		};
		const std::vector<VkVertexInputAttributeDescription> vertexAttributes = {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, (u32)offsetof(Vertex, Position) },
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, (u32)offsetof(Vertex, Normal) },
			{ 2, 0, VK_FORMAT_R32G32_SFLOAT, (u32)offsetof(Vertex, TexCoord0) },
			{ 3, 0, VK_FORMAT_R32G32_SFLOAT, (u32)offsetof(Vertex, TexCoord1) },
			{ 4, 0, VK_FORMAT_R32G32B32_SFLOAT, (u32)offsetof(Vertex, Tangent) }
		};

		auto geometryVert = LoadShader("vk_deferred_geometry.vert.spv", ShaderStage::Vertex);
		auto geometryFrag = LoadShader("vk_deferred_geometry.frag.spv", ShaderStage::Fragment);
		PipelineConfig geometry{};
		geometry.colorFormats = { GPositionFormat, GNormalFormat, GAlbedoFormat, GEmissiveFormat };
		geometry.depthFormat = GfxContext::Get().GetDepthFormat();
		geometry.depthTest = true;
		geometry.depthWrite = true;
		geometry.cullMode = VK_CULL_MODE_BACK_BIT;
		geometry.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		geometry.vertexBindings = vertexBindings;
		geometry.vertexAttributes = vertexAttributes;
		m_GeometryPipeline = std::make_unique<GfxPipeline>(geometry, geometryVert, geometryFrag,
			std::vector<VkDescriptorSetLayout>{ m_FrameSetLayout, m_TextureSetLayout, m_TextureSetLayout });

		auto fullscreenVert = LoadShader("vk_fullscreen.vert.spv", ShaderStage::Vertex);
		PipelineConfig fullscreen{};
		fullscreen.depthTest = false;
		fullscreen.depthWrite = false;
		fullscreen.cullMode = VK_CULL_MODE_NONE;

		fullscreen.colorFormats = { SsaoFormat };
		m_SsaoPipeline = std::make_unique<GfxPipeline>(fullscreen, fullscreenVert,
			LoadShader("vk_ssao.frag.spv", ShaderStage::Fragment), m_SsaoSetLayout);
		m_SsaoBlurPipeline = std::make_unique<GfxPipeline>(fullscreen, fullscreenVert,
			LoadShader("vk_ssao_blur.frag.spv", ShaderStage::Fragment), m_SingleTextureSetLayout);

		fullscreen.colorFormats = { HdrFormat };
		m_LightingPipeline = std::make_unique<GfxPipeline>(fullscreen, fullscreenVert,
			LoadShader("vk_deferred_lighting.frag.spv", ShaderStage::Fragment), m_LightingSetLayout);
		m_BloomExtractPipeline = std::make_unique<GfxPipeline>(fullscreen, fullscreenVert,
			LoadShader("vk_bloom_extract.frag.spv", ShaderStage::Fragment), m_BloomExtractSetLayout);

		VkPushConstantRange blurRange{};
		blurRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		blurRange.offset = 0;
		blurRange.size = sizeof(BlurPushConstants);
		fullscreen.pushConstants = { blurRange };
		m_BloomBlurPipeline = std::make_unique<GfxPipeline>(fullscreen, fullscreenVert,
			LoadShader("vk_bloom_blur.frag.spv", ShaderStage::Fragment), m_SingleTextureSetLayout);
		fullscreen.pushConstants.clear();

		fullscreen.colorFormats = { FinalFormat };
		m_CompositePipeline = std::make_unique<GfxPipeline>(fullscreen, fullscreenVert,
			LoadShader("vk_composite.frag.spv", ShaderStage::Fragment), m_CompositeSetLayout);

		auto transparentVert = LoadShader("vk_transparent.vert.spv", ShaderStage::Vertex);
		auto transparentFrag = LoadShader("vk_transparent.frag.spv", ShaderStage::Fragment);
		PipelineConfig transparent{};
		transparent.colorFormats = { HdrFormat };
		transparent.depthFormat = GfxContext::Get().GetDepthFormat();
		transparent.depthTest = true;
		transparent.depthWrite = false;
		transparent.blend = true;
		transparent.cullMode = VK_CULL_MODE_BACK_BIT;
		transparent.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		transparent.vertexBindings = vertexBindings;
		transparent.vertexAttributes = vertexAttributes;
		m_TransparentPipeline = std::make_unique<GfxPipeline>(transparent, transparentVert, transparentFrag,
			std::vector<VkDescriptorSetLayout>{ m_FrameSetLayout, m_TextureSetLayout, m_TextureSetLayout });
	}

	void VulkanDeferredRenderer::DestroyPipelines()
	{
		m_CompositePipeline.reset();
		m_BloomBlurPipeline.reset();
		m_BloomExtractPipeline.reset();
		m_TransparentPipeline.reset();
		m_LightingPipeline.reset();
		m_SsaoBlurPipeline.reset();
		m_SsaoPipeline.reset();
		m_GeometryPipeline.reset();
	}

	VulkanDeferredRenderer::GpuMesh* VulkanDeferredRenderer::GetOrCreateMesh(const Mesh* mesh)
	{
		if (!mesh) return nullptr;
		auto found = m_MeshCache.find(mesh);
		if (found != m_MeshCache.end()) return &found->second;

		const NullMesh* cpuMesh = dynamic_cast<const NullMesh*>(mesh);
		if (!cpuMesh)
		{
			EQN_CORE_ERROR("VulkanDeferredRenderer: expected a CPU/NullMesh for Vulkan upload");
			return nullptr;
		}

		auto vertexBuffer = cpuMesh->GetVertexBuffer();
		auto indexBuffer = cpuMesh->GetIndexBuffer();
		const auto* cpuVertices = vertexBuffer ? dynamic_cast<const NullVertexBuffer*>(vertexBuffer.get()) : nullptr;
		const auto* cpuIndices = indexBuffer ? dynamic_cast<const NullIndexBuffer*>(indexBuffer.get()) : nullptr;
		if (!cpuVertices || !cpuIndices || cpuVertices->GetData().empty() || cpuIndices->GetIndices().empty())
			return nullptr;

		if (cpuVertices->GetLayout().GetStride() != sizeof(Vertex))
		{
			EQN_CORE_WARN("VulkanDeferredRenderer: vertex stride {0}, expected {1}",
				cpuVertices->GetLayout().GetStride(), sizeof(Vertex));
			return nullptr;
		}

		GpuMesh gpu{};
		gpu.vertices = CreateBufferWithData(cpuVertices->GetData().data(), cpuVertices->GetData().size(),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		gpu.indices = CreateBufferWithData(cpuIndices->GetIndices().data(),
			cpuIndices->GetIndices().size() * sizeof(u32), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		gpu.indexCount = (u32)cpuIndices->GetIndices().size();

		auto [iterator, inserted] = m_MeshCache.emplace(mesh, gpu);
		(void)inserted;
		return &iterator->second;
	}

	VulkanDeferredRenderer::GpuTexture* VulkanDeferredRenderer::GetWhiteTexture()
	{
		if (m_WhiteTexture.valid) return &m_WhiteTexture;
		const u8 white[] = { 255, 255, 255, 255 };
		m_WhiteTexture.image = CreateTextureRGBA8(white, 1, 1, false);
		m_WhiteTexture.set = AllocateTextureSet(m_WhiteTexture.image.view);
		m_WhiteTexture.valid = m_WhiteTexture.image.image != VK_NULL_HANDLE && m_WhiteTexture.set != VK_NULL_HANDLE;
		return &m_WhiteTexture;
	}

	VulkanDeferredRenderer::GpuTexture* VulkanDeferredRenderer::GetOrCreateTexture(const Texture* texture)
	{
		if (!texture) return GetWhiteTexture();
		auto found = m_TextureCache.find(texture);
		if (found != m_TextureCache.end()) return found->second.valid ? &found->second : GetWhiteTexture();

		const fs::path& path = texture->GetPath();
		if (path.empty() || !fs::exists(path))
		{
			m_TextureCache.emplace(texture, GpuTexture{});
			return GetWhiteTexture();
		}

		int width = 0;
		int height = 0;
		int channels = 0;
		stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
		if (!pixels)
		{
			EQN_CORE_WARN("VulkanDeferredRenderer: cannot decode texture '{0}', using white", path.string());
			m_TextureCache.emplace(texture, GpuTexture{});
			return GetWhiteTexture();
		}

		GpuTexture gpu{};
		gpu.image = CreateTextureRGBA8(pixels, (u32)width, (u32)height, true);
		stbi_image_free(pixels);
		gpu.set = AllocateTextureSet(gpu.image.view);
		gpu.valid = gpu.image.image != VK_NULL_HANDLE && gpu.set != VK_NULL_HANDLE;

		auto [iterator, inserted] = m_TextureCache.emplace(texture, gpu);
		(void)inserted;
		return iterator->second.valid ? &iterator->second : GetWhiteTexture();
	}

	void VulkanDeferredRenderer::DestroyResourceCaches()
	{
		for (auto& [mesh, gpu] : m_MeshCache)
		{
			(void)mesh;
			DestroyBuffer(gpu.vertices);
			DestroyBuffer(gpu.indices);
		}
		m_MeshCache.clear();

		for (auto& [texture, gpu] : m_TextureCache)
		{
			(void)texture;
			DestroyImage(gpu.image);
		}
		m_TextureCache.clear();
		DestroyImage(m_WhiteTexture.image);
		m_WhiteTexture = {};
	}

	void VulkanDeferredRenderer::BeginScene(u32 frameIndex, const SceneCamera& camera,
		const SceneLighting& lighting, const SceneEffects& effects, const Vec4& clearColor)
	{
		if (!m_Initialized || frameIndex >= FrameCount) return;
		ApplyPendingResize();

		m_Camera = camera;
		m_Lighting = lighting;
		m_Effects = effects;
		m_ClearColor = clearColor;
		m_DrawItems.clear();
		m_OpaqueItems.clear();
		m_TransparentItems.clear();
		UploadSceneUniform(frameIndex);
		m_SceneActive = true;
	}

	void VulkanDeferredRenderer::SubmitItem(const Equinox::Gfx::DrawItem& item)
	{
		if (!m_SceneActive || !item.mesh) return;
		if (m_DrawItems.size() >= MaxObjectsPerFrame)
		{
			EQN_CORE_WARN("VulkanDeferredRenderer: MaxObjectsPerFrame reached ({0})", MaxObjectsPerFrame);
			return;
		}
		m_DrawItems.push_back(item);
	}

	void VulkanDeferredRenderer::UploadSceneUniform(u32 frameIndex)
	{
		DeferredSceneUniform uniform{};
		uniform.view = m_Camera.view;
		uniform.projection = m_Camera.projection;
		uniform.viewProj = m_Camera.viewProj;
		uniform.cameraPos = Vec4(m_Camera.position, 1.0f);

		Vec3 towardsLight = -m_Lighting.direction;
		const float lightLength = glm::length(towardsLight);
		towardsLight = lightLength > 0.0001f ? towardsLight / lightLength : Vec3(0.0f, 1.0f, 0.0f);
		uniform.lightDirAmbient = Vec4(towardsLight, m_Lighting.ambient);
		uniform.lightColor = Vec4(m_Lighting.color, 1.0f);
		uniform.viewportTime = Vec4((float)m_Width, (float)m_Height, m_Effects.time, m_Effects.enabled ? 1.0f : 0.0f);
		uniform.ssao = Vec4(m_Effects.ssaoRadius, m_Effects.ssaoBias, m_Effects.ssaoIntensity, (float)SsaoKernelSize);
		uniform.bloom = Vec4(m_Effects.bloomThreshold, m_Effects.bloomStrength,
			(float)m_Effects.bloomPasses, (float)m_Effects.debugView);
		uniform.post0 = Vec4(m_Effects.exposure, m_Effects.contrast, m_Effects.saturation, (float)m_Effects.toneMapping);
		uniform.post1 = Vec4(m_Effects.vignetteAmount, m_Effects.vignetteHardness,
			m_Effects.grainAmount, m_Effects.aberrationOffset);
		uniform.post2 = Vec4(m_Effects.shadowBalance, 0.0f);
		uniform.post3 = Vec4(m_Effects.midtoneBalance, 0.0f);
		uniform.post4 = Vec4(m_Effects.highlightBalance, m_Effects.sharpness);
		uniform.clearColor = m_ClearColor;

		uniform.lightCounts = Vec4((float)m_Lighting.directionalCount, (float)m_Lighting.pointCount, 0.0f, 0.0f);
		for (u32 i = 0; i < m_Lighting.directionalCount && i < SceneLighting::MaxDirectionalLights; ++i)
		{
			uniform.dirDirections[i] = Vec4(m_Lighting.directional[i].direction, 0.0f);
			uniform.dirColorsIntensity[i] = Vec4(m_Lighting.directional[i].color, m_Lighting.directional[i].intensity);
		}
		for (u32 i = 0; i < m_Lighting.pointCount && i < SceneLighting::MaxPointLights; ++i)
		{
			uniform.pointPositionsRange[i] = Vec4(m_Lighting.point[i].position, m_Lighting.point[i].range);
			uniform.pointColorsIntensity[i] = Vec4(m_Lighting.point[i].color, m_Lighting.point[i].intensity);
		}

		std::memcpy(m_SceneUbo[frameIndex].mapped, &uniform, sizeof(uniform));
		FlushBuffer(m_SceneUbo[frameIndex]);
	}

	void VulkanDeferredRenderer::BuildDrawLists()
	{
		for (u32 i = 0; i < (u32)m_DrawItems.size(); ++i)
		{
			const DrawItem& item = m_DrawItems[i];
			// Cutout stays in the opaque/deferred pass; only Transparent/Fade use
			// blending.  Classifying from a scalar alpha alone breaks masked text.
			if (item.transparent)
				m_TransparentItems.push_back(i);
			else
				m_OpaqueItems.push_back(i);
		}

		const Vec3 cameraPosition = m_Camera.position;
		std::sort(m_TransparentItems.begin(), m_TransparentItems.end(), [&](u32 a, u32 b)
		{
			const Vec3 positionA = Vec3(m_DrawItems[a].model[3]);
			const Vec3 positionB = Vec3(m_DrawItems[b].model[3]);
			const Vec3 deltaA = positionA - cameraPosition;
			const Vec3 deltaB = positionB - cameraPosition;
			return glm::dot(deltaA, deltaA) > glm::dot(deltaB, deltaB);
		});
	}

	void VulkanDeferredRenderer::UploadObjects(u32 frameIndex)
	{
		const VkDeviceSize frameBase = VkDeviceSize(frameIndex) * MaxObjectsPerFrame * kObjectUniformAlignment;
		for (u32 i = 0; i < (u32)m_DrawItems.size(); ++i)
		{
			const DrawItem& item = m_DrawItems[i];
			ObjectUniform object{};
			object.model = item.model;
			object.color = item.color;
			object.params = item.params;
			object.lightDir = item.lightDir;
			object.lightColor = item.lightColor;
			object.emissive = item.emissive;
			object.alpha = Vec4(item.alphaCutoff, item.alphaTexture ? 1.0f : 0.0f,
				(float)item.renderMode, item.alphaFromDiffuse ? 1.0f : 0.0f);
			object.uvSets = Vec4((float)item.diffuseUvIndex, (float)item.alphaUvIndex, 0.0f, 0.0f);

			const VkDeviceSize offset = frameBase + VkDeviceSize(i) * kObjectUniformAlignment;
			std::memcpy((u8*)m_ObjectUbo.mapped + offset, &object, sizeof(object));
		}
		if (!m_DrawItems.empty()) FlushBuffer(m_ObjectUbo);
	}

	bool VulkanDeferredRenderer::DrawMesh(VkCommandBuffer cmd, u32 frameIndex, u32 itemIndex,
		const GfxPipeline& pipeline, FrameStats& stats)
	{
		const DrawItem& item = m_DrawItems[itemIndex];
		GpuMesh* mesh = GetOrCreateMesh(item.mesh);
		GpuTexture* texture = GetOrCreateTexture(item.texture);
		GpuTexture* alphaTexture = GetOrCreateTexture(item.alphaTexture);
		if (!mesh || !texture || !texture->valid || !alphaTexture || !alphaTexture->valid) return false;

		VkDeviceSize vertexOffset = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &mesh->vertices.buffer, &vertexOffset);
		vkCmdBindIndexBuffer(cmd, mesh->indices.buffer, 0, VK_INDEX_TYPE_UINT32);

		const VkDeviceSize frameBase = VkDeviceSize(frameIndex) * MaxObjectsPerFrame * kObjectUniformAlignment;
		const VkDeviceSize objectOffset = frameBase + VkDeviceSize(itemIndex) * kObjectUniformAlignment;
		const u32 dynamicOffset = (u32)objectOffset;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(),
			0, 1, &m_FrameSets[frameIndex], 1, &dynamicOffset);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(),
			1, 1, &texture->set, 0, nullptr);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(),
			2, 1, &alphaTexture->set, 0, nullptr);
		vkCmdDrawIndexed(cmd, mesh->indexCount, 1, 0, 0, 0);
		stats.drawCalls++;
		return true;
	}

	void VulkanDeferredRenderer::RecordGeometryPass(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats)
	{
		EQN_PROFILE_SCOPE("VK Geometry pass");
		TransitionImage(cmd, m_GPositionRoughness, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		TransitionImage(cmd, m_GNormalMetallic, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		TransitionImage(cmd, m_GAlbedoAlpha, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		TransitionImage(cmd, m_GEmissive, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		TransitionImage(cmd, m_Depth, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		BeginRendering(cmd,
			{ &m_GPositionRoughness, &m_GNormalMetallic, &m_GAlbedoAlpha, &m_GEmissive },
			VK_ATTACHMENT_LOAD_OP_CLEAR, Vec4(0.0f), &m_Depth,
			VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
		SetViewportAndScissor(cmd, m_Width, m_Height, true);

		if (m_GeometryPipeline && m_GeometryPipeline->GetPipeline())
		{
			m_GeometryPipeline->Bind(cmd);
			for (u32 item : m_OpaqueItems)
				DrawMesh(cmd, frameIndex, item, *m_GeometryPipeline, stats);
		}
		vkCmdEndRendering(cmd);

		TransitionImage(cmd, m_GPositionRoughness, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		TransitionImage(cmd, m_GNormalMetallic, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		TransitionImage(cmd, m_GAlbedoAlpha, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		TransitionImage(cmd, m_GEmissive, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void VulkanDeferredRenderer::RecordSsaoPass(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats)
	{
		EQN_PROFILE_SCOPE("VK SSAO pass");
		TransitionImage(cmd, m_SsaoRaw, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		BeginRendering(cmd, { &m_SsaoRaw }, VK_ATTACHMENT_LOAD_OP_CLEAR, Vec4(1.0f));
		SetViewportAndScissor(cmd, m_Width, m_Height, false);

		if (m_SsaoPipeline && m_SsaoPipeline->GetPipeline())
		{
			m_SsaoPipeline->Bind(cmd);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SsaoPipeline->GetLayout(),
				0, 1, &m_SsaoSets[frameIndex], 0, nullptr);
			vkCmdDraw(cmd, 3, 1, 0, 0);
			stats.drawCalls++;
		}
		vkCmdEndRendering(cmd);
		TransitionImage(cmd, m_SsaoRaw, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void VulkanDeferredRenderer::RecordSsaoBlurPass(VkCommandBuffer cmd, FrameStats& stats)
	{
		EQN_PROFILE_SCOPE("VK SSAO blur pass");
		TransitionImage(cmd, m_SsaoBlur, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		BeginRendering(cmd, { &m_SsaoBlur }, VK_ATTACHMENT_LOAD_OP_CLEAR, Vec4(1.0f));
		SetViewportAndScissor(cmd, m_Width, m_Height, false);

		if (m_SsaoBlurPipeline && m_SsaoBlurPipeline->GetPipeline())
		{
			m_SsaoBlurPipeline->Bind(cmd);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SsaoBlurPipeline->GetLayout(),
				0, 1, &m_SsaoBlurSet, 0, nullptr);
			vkCmdDraw(cmd, 3, 1, 0, 0);
			stats.drawCalls++;
		}
		vkCmdEndRendering(cmd);
		TransitionImage(cmd, m_SsaoBlur, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void VulkanDeferredRenderer::RecordLightingPass(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats)
	{
		EQN_PROFILE_SCOPE("VK deferred lighting pass");
		TransitionImage(cmd, m_HdrColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		BeginRendering(cmd, { &m_HdrColor }, VK_ATTACHMENT_LOAD_OP_CLEAR, m_ClearColor);
		SetViewportAndScissor(cmd, m_Width, m_Height, false);

		if (m_LightingPipeline && m_LightingPipeline->GetPipeline())
		{
			m_LightingPipeline->Bind(cmd);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LightingPipeline->GetLayout(),
				0, 1, &m_LightingSets[frameIndex], 0, nullptr);
			vkCmdDraw(cmd, 3, 1, 0, 0);
			stats.drawCalls++;
		}
		vkCmdEndRendering(cmd);
	}

	void VulkanDeferredRenderer::RecordTransparentPass(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats)
	{
		EQN_PROFILE_SCOPE("VK transparent forward pass");
		if (!m_TransparentItems.empty())
		{
			BeginRendering(cmd, { &m_HdrColor }, VK_ATTACHMENT_LOAD_OP_LOAD, Vec4(0.0f), &m_Depth,
				VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
			SetViewportAndScissor(cmd, m_Width, m_Height, true);

			if (m_TransparentPipeline && m_TransparentPipeline->GetPipeline())
			{
				m_TransparentPipeline->Bind(cmd);
				for (u32 item : m_TransparentItems)
					DrawMesh(cmd, frameIndex, item, *m_TransparentPipeline, stats);
			}
			vkCmdEndRendering(cmd);
		}
		TransitionImage(cmd, m_HdrColor, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void VulkanDeferredRenderer::RecordBloomPasses(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats)
	{
		EQN_PROFILE_SCOPE("VK bloom passes");
		const u32 extractWidth = m_BloomExtract.extent.width;
		const u32 extractHeight = m_BloomExtract.extent.height;
		const u32 halfWidth = m_BloomPing[0].extent.width;
		const u32 halfHeight = m_BloomPing[0].extent.height;

		TransitionImage(cmd, m_BloomExtract, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		BeginRendering(cmd, { &m_BloomExtract }, VK_ATTACHMENT_LOAD_OP_CLEAR, Vec4(0.0f));
		SetViewportAndScissor(cmd, extractWidth, extractHeight, false);
		if (m_BloomExtractPipeline && m_BloomExtractPipeline->GetPipeline())
		{
			m_BloomExtractPipeline->Bind(cmd);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BloomExtractPipeline->GetLayout(),
				0, 1, &m_BloomExtractSets[frameIndex], 0, nullptr);
			vkCmdDraw(cmd, 3, 1, 0, 0);
			stats.drawCalls++;
		}
		vkCmdEndRendering(cmd);
		TransitionImage(cmd, m_BloomExtract, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		const int passCount = glm::clamp(m_Effects.bloomPasses, 1, 16) * 2;
		for (int pass = 0; pass < passCount; ++pass)
		{
			const i32 destination = pass % 2;
			const i32 sourceSet = pass == 0 ? 0 : 1 + ((pass - 1) % 2);
			TransitionImage(cmd, m_BloomPing[destination], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			BeginRendering(cmd, { &m_BloomPing[destination] }, VK_ATTACHMENT_LOAD_OP_CLEAR, Vec4(0.0f));
			SetViewportAndScissor(cmd, halfWidth, halfHeight, false);

			if (m_BloomBlurPipeline && m_BloomBlurPipeline->GetPipeline())
			{
				m_BloomBlurPipeline->Bind(cmd);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_BloomBlurPipeline->GetLayout(),
					0, 1, &m_BloomBlurSets[sourceSet], 0, nullptr);

				BlurPushConstants push{};
				const float sourceWidth = pass == 0 ? (float)extractWidth : (float)halfWidth;
				const float sourceHeight = pass == 0 ? (float)extractHeight : (float)halfHeight;
				push.texelSize = Vec2(1.0f / sourceWidth, 1.0f / sourceHeight);
				push.horizontal = (pass & 1) == 0 ? 1 : 0;
				push.strength = m_Effects.bloomStrength;
				vkCmdPushConstants(cmd, m_BloomBlurPipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT,
					0, sizeof(push), &push);
				vkCmdDraw(cmd, 3, 1, 0, 0);
				stats.drawCalls++;
			}
			vkCmdEndRendering(cmd);
			TransitionImage(cmd, m_BloomPing[destination], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			m_LastBloomImage = destination;
		}
	}

	void VulkanDeferredRenderer::RecordCompositePass(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats)
	{
		EQN_PROFILE_SCOPE("VK composite pass");
		TransitionImage(cmd, m_FinalColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		BeginRendering(cmd, { &m_FinalColor }, VK_ATTACHMENT_LOAD_OP_CLEAR, Vec4(0.0f));
		SetViewportAndScissor(cmd, m_Width, m_Height, false);

		if (m_CompositePipeline && m_CompositePipeline->GetPipeline())
		{
			m_CompositePipeline->Bind(cmd);
			VkDescriptorSet set = m_CompositeSets[frameIndex][m_LastBloomImage];
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CompositePipeline->GetLayout(),
				0, 1, &set, 0, nullptr);
			vkCmdDraw(cmd, 3, 1, 0, 0);
			stats.drawCalls++;
		}
		vkCmdEndRendering(cmd);
		TransitionImage(cmd, m_FinalColor, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void VulkanDeferredRenderer::EndScene(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats)
	{
		if (!m_SceneActive || frameIndex >= FrameCount) return;
		EQN_PROFILE_SCOPE("VulkanDeferredRenderer::EndScene");

		BuildDrawLists();
		UploadObjects(frameIndex);

		stats.opaqueObjects = (u32)m_OpaqueItems.size();
		stats.transparentObjects = (u32)m_TransparentItems.size();
		stats.geometryDrawCalls = (u32)m_OpaqueItems.size();
		stats.transparentDrawCalls = (u32)m_TransparentItems.size();
		const u32 bloomBlurPasses = (u32)glm::clamp(m_Effects.bloomPasses, 1, 16) * 2u;
		stats.bloomPasses = 1u + bloomBlurPasses; // extraction + blur passes
		stats.fullscreenPasses = 5u + bloomBlurPasses; // SSAO, blur, light, bloom, composite

		RecordGeometryPass(cmd, frameIndex, stats);
		RecordSsaoPass(cmd, frameIndex, stats);
		RecordSsaoBlurPass(cmd, stats);
		RecordLightingPass(cmd, frameIndex, stats);
		RecordTransparentPass(cmd, frameIndex, stats);
		RecordBloomPasses(cmd, frameIndex, stats);
		RecordCompositePass(cmd, frameIndex, stats);

		stats.cachedMeshes = (u32)m_MeshCache.size();
		stats.cachedTextures = (u32)m_TextureCache.size() + (m_WhiteTexture.valid ? 1u : 0u);
		m_SceneActive = false;
	}
}

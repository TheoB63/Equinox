#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/graphics/GfxContext.h"
#include "equinox/graphics/GfxShader.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Equinox::Gfx
{
	struct PipelineConfig
	{
		// Dynamic rendering: the pipeline declares formats, not a VkRenderPass.
		std::vector<VkFormat> colorFormats;
		VkFormat depthFormat = VK_FORMAT_UNDEFINED;

		bool depthTest = true;
		bool depthWrite = true;
		bool blend = false;

		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		// Empty for a full-screen triangle generated from gl_VertexIndex.
		std::vector<VkVertexInputBindingDescription> vertexBindings;
		std::vector<VkVertexInputAttributeDescription> vertexAttributes;

		std::vector<VkPushConstantRange> pushConstants;
	};

	class GfxPipeline
	{
	public:
		GfxPipeline(const PipelineConfig& config,
			const std::shared_ptr<GfxShader>& vertShader,
			const std::shared_ptr<GfxShader>& fragShader,
			const std::vector<VkDescriptorSetLayout>& descriptorLayouts = {});

		// Convenience overload kept for existing call sites.
		GfxPipeline(const PipelineConfig& config,
			const std::shared_ptr<GfxShader>& vertShader,
			const std::shared_ptr<GfxShader>& fragShader,
			VkDescriptorSetLayout descriptorLayout);

		~GfxPipeline();

		GfxPipeline(const GfxPipeline&) = delete;
		GfxPipeline& operator=(const GfxPipeline&) = delete;

		void Bind(VkCommandBuffer cmd) const;
		VkPipelineLayout GetLayout() const { return m_Layout; }
		VkPipeline GetPipeline() const { return m_Pipeline; }

	private:
		void CreatePipeline(const PipelineConfig& config,
			const std::shared_ptr<GfxShader>& vertShader,
			const std::shared_ptr<GfxShader>& fragShader,
			const std::vector<VkDescriptorSetLayout>& descriptorLayouts);

		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_Layout = VK_NULL_HANDLE;
	};
}

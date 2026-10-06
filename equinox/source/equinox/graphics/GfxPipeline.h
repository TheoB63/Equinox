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
		// Formats of the images we draw into (dynamic rendering: no RenderPass)
		std::vector<VkFormat> colorFormats;
		VkFormat depthFormat = VK_FORMAT_UNDEFINED;

		bool depthTest = true;
		bool depthWrite = true;
		bool blend = false;

		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		// Vertex description (empty = no vertex buffer, e.g. the triangle hard-coded in the shader)
		std::vector<VkVertexInputBindingDescription> vertexBindings;
		std::vector<VkVertexInputAttributeDescription> vertexAttributes;

		// Small data sent directly to the shader (push constants)
		std::vector<VkPushConstantRange> pushConstants;
	};

	class GfxPipeline
	{
	public:
		GfxPipeline(const PipelineConfig& config,
			const std::shared_ptr<GfxShader>& vertShader,
			const std::shared_ptr<GfxShader>& fragShader,
			VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE);
		~GfxPipeline();

		GfxPipeline(const GfxPipeline&) = delete;
		GfxPipeline& operator=(const GfxPipeline&) = delete;

		void Bind(VkCommandBuffer cmd);
		VkPipelineLayout GetLayout() const { return m_Layout; }
		VkPipeline GetPipeline() const { return m_Pipeline; }

	private:
		void CreatePipeline(const PipelineConfig& config,
			const std::shared_ptr<GfxShader>& vertShader,
			const std::shared_ptr<GfxShader>& fragShader,
			VkDescriptorSetLayout descriptorLayout);

		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_Layout = VK_NULL_HANDLE;
	};
}
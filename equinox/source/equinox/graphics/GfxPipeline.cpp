#include "eqnpch.h"
#include "equinox/graphics/GfxPipeline.h"
#include "equinox/core/Log.h"

namespace Equinox::Gfx
{
	GfxPipeline::GfxPipeline(const PipelineConfig& config,
		const std::shared_ptr<GfxShader>& vertShader,
		const std::shared_ptr<GfxShader>& fragShader,
		const std::vector<VkDescriptorSetLayout>& descriptorLayouts)
	{
		CreatePipeline(config, vertShader, fragShader, descriptorLayouts);
	}

	GfxPipeline::GfxPipeline(const PipelineConfig& config,
		const std::shared_ptr<GfxShader>& vertShader,
		const std::shared_ptr<GfxShader>& fragShader,
		VkDescriptorSetLayout descriptorLayout)
	{
		std::vector<VkDescriptorSetLayout> layouts;
		if (descriptorLayout != VK_NULL_HANDLE)
			layouts.push_back(descriptorLayout);
		CreatePipeline(config, vertShader, fragShader, layouts);
	}

	GfxPipeline::~GfxPipeline()
	{
		auto device = GfxContext::Get().GetDevice();
		if (m_Pipeline) vkDestroyPipeline(device, m_Pipeline, nullptr);
		if (m_Layout) vkDestroyPipelineLayout(device, m_Layout, nullptr);
	}

	void GfxPipeline::Bind(VkCommandBuffer cmd) const
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	}

	void GfxPipeline::CreatePipeline(const PipelineConfig& config,
		const std::shared_ptr<GfxShader>& vertShader,
		const std::shared_ptr<GfxShader>& fragShader,
		const std::vector<VkDescriptorSetLayout>& descriptorLayouts)
	{
		auto device = GfxContext::Get().GetDevice();

		if (!vertShader || !fragShader || !vertShader->GetModule() || !fragShader->GetModule())
		{
			EQN_CORE_ERROR("GfxPipeline: shader module missing, pipeline not created");
			return;
		}

		VkPipelineShaderStageCreateInfo stages[2]{};
		stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stages[0].module = vertShader->GetModule();
		stages[0].pName = vertShader->GetEntryPoint().c_str();
		stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stages[1].module = fragShader->GetModule();
		stages[1].pName = fragShader->GetEntryPoint().c_str();

		VkPipelineVertexInputStateCreateInfo vertexInput{};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = (u32)config.vertexBindings.size();
		vertexInput.pVertexBindingDescriptions = config.vertexBindings.empty() ? nullptr : config.vertexBindings.data();
		vertexInput.vertexAttributeDescriptionCount = (u32)config.vertexAttributes.size();
		vertexInput.pVertexAttributeDescriptions = config.vertexAttributes.empty() ? nullptr : config.vertexAttributes.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = config.topology;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo raster{};
		raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		raster.polygonMode = config.polygonMode;
		raster.cullMode = config.cullMode;
		raster.frontFace = config.frontFace;
		raster.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisample{};
		multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depth{};
		depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth.depthTestEnable = (config.depthTest && config.depthFormat != VK_FORMAT_UNDEFINED) ? VK_TRUE : VK_FALSE;
		depth.depthWriteEnable = (config.depthWrite && config.depthFormat != VK_FORMAT_UNDEFINED) ? VK_TRUE : VK_FALSE;
		depth.depthCompareOp = VK_COMPARE_OP_LESS;
		depth.depthBoundsTestEnable = VK_FALSE;
		depth.stencilTestEnable = VK_FALSE;

		std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(config.colorFormats.size());
		for (auto& attachment : blendAttachments)
		{
			attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			attachment.blendEnable = config.blend ? VK_TRUE : VK_FALSE;
			attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			attachment.colorBlendOp = VK_BLEND_OP_ADD;
			attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			attachment.alphaBlendOp = VK_BLEND_OP_ADD;
		}

		VkPipelineColorBlendStateCreateInfo colorBlend{};
		colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlend.attachmentCount = (u32)blendAttachments.size();
		colorBlend.pAttachments = blendAttachments.empty() ? nullptr : blendAttachments.data();

		const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamic{};
		dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic.dynamicStateCount = 2;
		dynamic.pDynamicStates = dynamicStates;

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = (u32)descriptorLayouts.size();
		layoutInfo.pSetLayouts = descriptorLayouts.empty() ? nullptr : descriptorLayouts.data();
		layoutInfo.pushConstantRangeCount = (u32)config.pushConstants.size();
		layoutInfo.pPushConstantRanges = config.pushConstants.empty() ? nullptr : config.pushConstants.data();
		if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
		{
			EQN_CORE_CRITICAL("Failed to create pipeline layout");
			return;
		}

		VkPipelineRenderingCreateInfo rendering{};
		rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		rendering.colorAttachmentCount = (u32)config.colorFormats.size();
		rendering.pColorAttachmentFormats = config.colorFormats.empty() ? nullptr : config.colorFormats.data();
		rendering.depthAttachmentFormat = config.depthFormat;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = &rendering;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = stages;
		pipelineInfo.pVertexInputState = &vertexInput;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &raster;
		pipelineInfo.pMultisampleState = &multisample;
		pipelineInfo.pDepthStencilState = &depth;
		pipelineInfo.pColorBlendState = &colorBlend;
		pipelineInfo.pDynamicState = &dynamic;
		pipelineInfo.layout = m_Layout;
		pipelineInfo.renderPass = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
			EQN_CORE_CRITICAL("Failed to create graphics pipeline");
	}
}

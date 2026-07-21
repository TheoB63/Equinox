#include "eqnpch.h"
#include "equinox/renderer/vulkan/VKFrameBuffer.h"
#include "equinox/renderer/vulkan/VKCommon.h"

namespace Equinox
{
	VKFramebuffer::VKFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView imageView, VkExtent2D extent)
		: m_Device(device)
	{
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &imageView;
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_FrameBuffer),
			"Failed to create framebuffer!");
	}

	VKFramebuffer::~VKFramebuffer()
	{
		vkDestroyFramebuffer(m_Device, m_FrameBuffer, nullptr);
	}
}
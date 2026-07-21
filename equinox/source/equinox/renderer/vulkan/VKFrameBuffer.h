#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Equinox
{
	class VKFramebuffer
	{
	public:
		VKFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView imageView, VkExtent2D extent);
		~VKFramebuffer();

		VkFramebuffer GetHandle() const { return m_FrameBuffer; }
	private:
		VkDevice m_Device;
		VkFramebuffer m_FrameBuffer;
	};
}
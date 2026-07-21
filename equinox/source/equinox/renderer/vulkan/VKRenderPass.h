#pragma once
#include <vulkan/vulkan.h>

namespace Equinox
{
	class VKRenderPass
	{
	public:
		VKRenderPass(VkDevice device, VkFormat swapchainFormat);
		~VKRenderPass();

		VkRenderPass GetHandle() const { return m_RenderPass; }

	private:
		VkDevice m_Device;
		VkRenderPass m_RenderPass;
	};
}
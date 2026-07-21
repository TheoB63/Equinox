#pragma once

#include <vulkan/vulkan.h>

namespace Equinox
{
	class VKCommandPool
	{
	public:
		VKCommandPool(VkDevice device, uint32_t queueFamilyIndex);
		~VKCommandPool();

		VkCommandPool GetHandle() const { return m_CommandPool; }

	private:
		VkDevice m_Device;
		VkCommandPool m_CommandPool;
	};
}
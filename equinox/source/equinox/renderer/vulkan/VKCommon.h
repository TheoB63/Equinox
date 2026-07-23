#pragma once

#include "equinox/core/Log.h"
#include "equinox/renderer/vulkan/VKFormat.h"

#define VK_CHECK_RESULT(result, message)                                          \
    do {                                                                          \
        VkResult vkRes = (result);                                                \
        if (vkRes != VK_SUCCESS) {                                                \
            EQN_CORE_ASSERT(false, "Vulkan Error: {0} (Code: {1})", message, static_cast<int>(vkRes)); \
        }                                                                          \
    } while (0)

namespace Equinox::VKUtils
{
    uint32_t FindMemoryType(VkPhysicalDevice physicalDevice,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties);

    void CreateBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer& buffer, VkDeviceMemory& memory);

    void CopyBuffer(VkBuffer src, VkBuffer dst, VkDevice device, VkDeviceSize size,
        VkQueue queue, uint32_t queueFamily);
}
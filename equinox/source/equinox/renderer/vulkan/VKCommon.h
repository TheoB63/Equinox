#pragma once

#include "equinox/core/Log.h"
#include "equinox/renderer/vulkan/VKFormat.h"

#define VK_CHECK_RESULT(result, message)                                          \
    do {                                                                          \
        VkResult vkRes = (result);                                                \
        if (vkRes != VK_SUCCESS) {                                                \
            LH_CORE_ASSERT(false, "Vulkan Error: {0} (Code: {1})", message, static_cast<int>(vkRes)); \
        }                                                                          \
    } while (0)
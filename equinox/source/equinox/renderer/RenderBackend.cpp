#include "eqnpch.h"
#include "RenderBackend.h"
#include "equinox/renderer/backend/vulkan/VulkanBackend.h"

namespace Equinox
{
    RenderBackend::API RenderBackend::s_API = RenderBackend::API::Vulkan;

    std::unique_ptr<RenderBackend> RenderBackend::Create()
    {
        switch (s_API)
        {
        case RenderBackend::API::None:    return nullptr;
        case RenderBackend::API::Vulkan:  return std::make_unique<VulkanBackend>();
        }
        return nullptr;
    }
}
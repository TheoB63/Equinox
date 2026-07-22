#include "eqnpch.h"
#include "equinox/renderer/VertexBuffer.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/vulkan/VKRendererAPI.h"
#include "equinox/renderer/openGL/GLVertexBuffer.h"
#include "equinox/renderer/vulkan/VKVertexBuffer.h"

#include <memory>

namespace Equinox
{
    std::unique_ptr<VertexBuffer> VertexBuffer::Create(uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None:
            EQN_CORE_ASSERT(false, "RendererAPI::None is not supported!");
            return nullptr;

        case RendererAPI::API::OpenGL:
            return std::make_unique<GLVertexBuffer>(size);

        case RendererAPI::API::Vulkan:
        {
            auto vkRenderer = static_cast<VKRendererAPI*>(Renderer::GetRendererAPI());
            return std::make_unique<VKVertexBuffer>(
                vkRenderer->GetLogicalDevice(),
                vkRenderer->GetPhysicalDevice(),
                size
            );
        }
        }

        EQN_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

    std::unique_ptr<VertexBuffer> VertexBuffer::Create(const void* data, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::None:
            EQN_CORE_ASSERT(false, "RendererAPI::None is not supported!");
            return nullptr;

        case RendererAPI::API::OpenGL:
            return std::make_unique<GLVertexBuffer>(data, size);

        case RendererAPI::API::Vulkan:
        {
            auto vkRenderer = static_cast<VKRendererAPI*>(Renderer::GetRendererAPI());
            return std::make_unique<VKVertexBuffer>(
                vkRenderer->GetLogicalDevice(),
                vkRenderer->GetPhysicalDevice(),
                data,
                size
            );
        }
        }

        EQN_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }
}
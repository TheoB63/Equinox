#include "eqnpch.h"
#include "equinox/renderer/IndexBuffer.h"
#include "equinox/renderer/Renderer.h"

#include "equinox/renderer/OpenGL/GLVertexArray.h"
#include "equinox/renderer/OpenGL/GLIndexBuffer.h"

#include "equinox/renderer/Vulkan/VKRendererAPI.h"
#include "equinox/renderer/Vulkan/VKVertexArray.h"
#include "equinox/renderer/Vulkan/VKIndexBuffer.h"

namespace Equinox
{
    std::unique_ptr<IndexBuffer> IndexBuffer::Create(const uint32_t* indices, uint32_t count)
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::OpenGL:
            return std::make_unique<GLIndexBuffer>(indices, count);

        case RendererAPI::API::Vulkan:
        {
            auto vkRenderer = static_cast<VKRendererAPI*>(Renderer::GetRendererAPI());
            return std::make_unique<VKIndexBuffer>(
                vkRenderer->GetLogicalDevice(),
                vkRenderer->GetPhysicalDevice(),
                indices,
                count
            );
        }

        default:
            EQN_CORE_ASSERT(false, "Unknown RendererAPI!");
            return nullptr;
        }
    }
}
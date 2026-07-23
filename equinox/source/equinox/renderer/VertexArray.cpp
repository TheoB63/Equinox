#include "eqnpch.h"
#include "equinox/renderer/VertexArray.h"
#include "equinox/renderer/Renderer.h"

#include "equinox/renderer/OpenGL/GLVertexArray.h"

#include "equinox/renderer/Vulkan/VKRendererAPI.h"
#include "equinox/renderer/Vulkan/VKVertexArray.h"

namespace Equinox
{
    std::unique_ptr<VertexArray> VertexArray::Create()
    {
        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::OpenGL:
            return std::make_unique<GLVertexArray>();

        case RendererAPI::API::Vulkan:
        {
            auto vkRenderer = static_cast<VKRendererAPI*>(Renderer::GetRendererAPI());
            return std::make_unique<VKVertexArray>(vkRenderer->GetLogicalDevice().GetHandle());
        }

        default:
            EQN_CORE_ASSERT(false, "Unknown RendererAPI!");
            return nullptr;
        }
    }
}
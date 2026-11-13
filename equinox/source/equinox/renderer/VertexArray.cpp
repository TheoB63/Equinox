#include "eqnpch.h"
#include "equinox/renderer/VertexArray.h"
#include "equinox/renderer/Renderer.h"

#include "equinox/renderer/backend/opengl/GLVertexArray.h"

#include "equinox/renderer/null/NullResources.h"

namespace Equinox
{
    std::unique_ptr<VertexArray> VertexArray::Create()
    {
        switch (Renderer::GetBackend()->GetAPI())
        {
        case RenderBackend::API::OpenGL:
            return std::make_unique<GLVertexArray>();

        case RenderBackend::API::Vulkan:
            return std::make_unique<NullVertexArray>();

        default:
            EQN_CORE_ASSERT(false, "Unknown RendererAPI!");
            return nullptr;
        }
    }
}

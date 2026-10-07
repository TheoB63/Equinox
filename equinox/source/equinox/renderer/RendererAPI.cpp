#include "eqnpch.h"
#include "equinox/renderer/RendererAPI.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/backend/vulkan/VKRendererAPI.h"
#include "equinox/renderer/backend/opengl/GLRendererAPI.h"

namespace Equinox
{
    RendererAPI::API RendererAPI::s_API = API::Vulkan;

    // [OP] Dual-backend: create the requested backend implementation.
    std::unique_ptr<RendererAPI> RendererAPI::Create(API api)
    {
        s_API = api;

        switch (s_API)
        {
            case RendererAPI::API::None:
                EQN_CORE_ASSERT(false, "RendererAPI::None is not supported!");
                return nullptr;

            case RendererAPI::API::OpenGL:
                return std::make_unique<GLRendererAPI>();

            case RendererAPI::API::Vulkan:
                return std::make_unique<VKRendererAPI>();

            default:
                EQN_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }
}

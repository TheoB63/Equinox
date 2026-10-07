#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/renderer/RendererAPI.h"

#include <memory>

namespace Equinox
{
    namespace RG { class RenderGraph; }

    class Renderer
    {
    public:
        // [OP] Dual-backend: choose the renderer API (default: Vulkan).
        static void Init(void* windowHandle, RendererAPI::API api = RendererAPI::API::Vulkan);
        static void Shutdown();

        static bool BeginFrame();
        static void EndFrame();
        
        static void ExecuteGraph(RG::RenderGraph& graph);
        static void OnResize(u32 width, u32 height);

        static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
        static RendererAPI* GetRendererAPI() { return s_RendererAPI.get(); }

    private:
        static std::unique_ptr<RendererAPI> s_RendererAPI;
    };
}

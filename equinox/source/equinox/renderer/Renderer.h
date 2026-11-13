#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/renderer/RenderBackend.h"
#include "equinox/core/FrameData.h"

#include <memory>

namespace Equinox
{
	namespace RG { class RenderGraph; }

	class Renderer
	{
	public:
		// [OP] Dual-backend: choose the renderer API (default: Vulkan).
		static void Init(void* windowHandle);
		static void Shutdown();

		static void BeginFrame();
		static void EndFrame();

		static void ExecuteGraph(RG::RenderGraph& graph);
		static void OnResize(u32 width, u32 height);

		static RenderBackend* GetBackend() { return s_Backend.get(); }
		static FrameContext& GetCurrentFrame() { return s_FrameData.GetCurrentFrame(); }

	private:
		static std::unique_ptr<RenderBackend> s_Backend;
		static FrameData s_FrameData;
	};
}

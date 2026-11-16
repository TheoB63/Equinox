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

		// Frame management — Renderer uses FrameData owned by App
		static void SetFrameData(FrameData* frameData);

		// Frame Logic
		static void BeginFrame(u64 frameIndex);
		static void EndFrame();

		static void ExecuteGraph(RG::RenderGraph& graph, u64 frameIndex);
		static void OnResize(u32 width, u32 height);

		static RenderBackend* GetBackend() { return s_Backend.get(); }
		static FrameData* GetFrameData() { return s_FrameData; }

	private:
		static std::unique_ptr<RenderBackend> s_Backend;
		static FrameData* s_FrameData;
	};
}

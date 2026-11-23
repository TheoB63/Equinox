#pragma once

#include "equinox/editor/Editor.h"
#include "equinox/scene/systems/RenderingSystem.h"

#include <memory>

namespace Equinox
{
	class FrameDebuggerPanel : public Panel
	{
	public:
		void OnInit() override;
		void OnRender() override;

	private:
		void DrawControlBar(const RG::RenderGraphSnapshot& snapshot, int nonCulledCount);
		void DrawPassTree(const RG::RenderGraphSnapshot& snapshot);
		void DrawPassDetails(const RG::RenderGraphSnapshot& snapshot);

		std::shared_ptr<RenderingSystem> m_RS;
		int  m_SelectedPassIndex = -1;
		int  m_SelectedResourceIndex = -1;
		int  m_EventSliderValue = -1;   // -1 = show all, 0..N-1 = selected non-culled pass
		bool m_Enabled = true;
	};
}
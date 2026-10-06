#pragma once

#include "equinox/editor/Editor.h"

namespace Equinox
{
	// Engine-owned metrics window.  Registering it in Editor gives EquinoxApp,
	// Sandbox OpenGL, Sandbox Vulkan and RTShader the same diagnostics.
	class MetricsPanel final : public Panel
	{
	public:
		void OnInit() override;
		void OnRender() override;

	private:
		bool m_VSync = true;
	};
}

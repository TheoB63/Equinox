#include <Equinox.h>
#include <Equinox/core/EntryPoint.h>

#include <equinox/graphics/GfxRenderer.h>
#include <equinox/graphics/GfxScene.h>

#include <imgui.h>

namespace Equinox
{
	class EquinoxApp : public App
	{
	public:
		EquinoxApp(int argc, char** argv) : App(argc, argv) {}
		~EquinoxApp() override = default;

	protected:
		void OnInit() override {}

		void OnUpdate() override
		{
		}

		void OnUIRender() override
		{
			// Equinox Metrics is engine-owned (MetricsPanel), so every application
			// gets exactly the same window without app-local duplicates.
			static bool showDemo = true;
			if (showDemo) ImGui::ShowDemoWindow(&showDemo);
		}

		void OnShutdown() override {}
	};

	App* CreateApp(int argc, char** argv)
	{
		return new EquinoxApp(argc,argv);
	}
}
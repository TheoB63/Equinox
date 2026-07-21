#include <Equinox.h>
#include <Equinox/core/EntryPoint.h>

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

		void OnUpdate(f32 dt) override
		{
			static float time = 0;
			time += dt;
		}

		void OnUIRender() override
		{
			ImGui::Begin("Equinox Dashboard");
			//ImGui::Text("Welcome to Equinox!");
			ImGui::End();

			// ImGui Demo Window
			static bool showDemo = true;
			if (showDemo)
			{
				ImGui::ShowDemoWindow(&showDemo);
			}
		}

		void OnShutdown() override {}
	};

	App* CreateApp(int argc, char** argv)
	{
		return new EquinoxApp(argc,argv);
	}
}
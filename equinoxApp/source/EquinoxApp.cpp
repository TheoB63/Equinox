#include "Equinox.h"

#include <imgui.h>

namespace Equinox
{
	class EquinoxApp : public App
	{
	public:
		EquinoxApp() {}
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

	App* createApp()
	{
		return new EquinoxApp();
	}
}

int main()
{
	Equinox::Log::Init();
	Equinox::App* app = Equinox::createApp();
	app->Run();
	delete app;
	return 0;
}
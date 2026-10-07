#include <Equinox.h>
#include <equinox/core/EntryPoint.h>

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
            // ImGui Demo
            static bool showDemo = true;
            if (showDemo) ImGui::ShowDemoWindow(&showDemo);

            ImGuiIO& io = ImGui::GetIO();
            ImGui::Begin("Equinox Metrics");
            ImGui::Text("Frame time %.3f ms (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        void OnShutdown() override {}
    };

    App* CreateApp(int argc, char** argv)
    {
        return new EquinoxApp(argc, argv);
    }
}

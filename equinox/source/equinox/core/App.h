#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/window/Window.h"
#include "equinox/events/EventBus.h"
#include "equinox/events/AppEvent.h"
#include "equinox/events/FileDropEvent.h"
#include "equinox/ECS/Scene.h"

#include <vector>
#include <memory>

namespace Equinox
{
    class App
    {
    public:
        App(int argc, char** argv);
        virtual ~App();

        void Run();
        void Close();

        WindowSpec ParseCommandLineArgs(int argc, char** argv);
        Window& GetWindow() { return *m_Window; }

    protected:
        virtual void OnInit() {}
        virtual void OnUpdate() {}
        virtual void OnUIRender() {}
        virtual void OnShutdown() {}

    private:
        void SetAppTitle(WindowSpec& ws);

        void OnWindowResize(WindowResizeEvent& e);
        void OnWindowClose(WindowCloseEvent& e);
        void OnFileDrop(FileDropEvent& e);

        std::shared_ptr<Window> m_Window;
        std::shared_ptr<Scene> m_Scene;

        bool m_Running = true;
    };

    App* CreateApp(int argc, char** argv);
}

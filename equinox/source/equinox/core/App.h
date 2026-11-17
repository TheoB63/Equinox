#pragma once

#include "equinox/core/EquinoxTypes.h"

#include "equinox/platform/Window.h"
#include "equinox/platform/EventBus.h"
#include "equinox/platform/AppEvent.h"
#include "equinox/platform/FileDropEvent.h"

#include "equinox/scene/Scene.h"

#include "equinox/renderer/FrameData.h"

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

		// Frame Pipelining
		FrameData m_FrameData;

		bool m_Running = true;
	};

	App* CreateApp(int argc, char** argv);
}

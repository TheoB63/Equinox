#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/window/Window.h"

#include "equinox/events/EventBus.h"
#include "equinox/events/AppEvent.h"
#include "equinox/events/FileDropEvent.h"

#include <vector>

namespace Equinox
{
	typedef unsigned int GLuint;

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
		void OnWindowResize(WindowResizeEvent& e);
		void OnWindowClose(WindowCloseEvent& e);
		void OnFileDrop(FileDropEvent& e);

	private:

		std::unique_ptr<Window> m_Window;
		std::shared_ptr<EventBus> m_MainThreadEventBus;

		bool m_Running = true;
		f32 m_LastFrameTime = 0.0f;

		GLuint quadVAO, quadVBO;
	};

	App* CreateApp(int argc, char** argv);
}
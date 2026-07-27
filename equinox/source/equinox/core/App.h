#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/window/Window.h"

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

		std::unique_ptr<Window> m_Window;

		bool m_Running = true;
		f32 m_LastFrameTime = 0.0f;

		GLuint quadVAO, quadVBO;
	};

	App* CreateApp(int argc, char** argv);
}
#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/window/Window.h"

#include <vector>

namespace Equinox
{
	class App
	{
	public:
		App();
		virtual ~App();

		void Run();

		void Close();

		Window& GetWindow() { return *m_Window; }

	protected:
		virtual void OnInit() {}
		virtual void OnUpdate(f32 dt) {}
		virtual void OnShutdown() {}

	private:

		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
		f32 m_LastFrameTime = 0.0f;
	};

	App* CreateApp();
}
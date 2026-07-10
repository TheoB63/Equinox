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
		App();
		virtual ~App();

		void Run();

		void Close();

		Window& GetWindow() { return *m_Window; }
		Renderer& GetRenderer() { return *m_Renderer; }

	protected:
		virtual void OnInit() {}
		virtual void OnUpdate(f32 dt) {}
		virtual void OnUIRender() {}
		virtual void OnShutdown() {}

	private:

		std::unique_ptr<Window> m_Window;
		std::unique_ptr<Renderer> m_Renderer;
		bool m_Running = true;
		f32 m_LastFrameTime = 0.0f;

		GLuint quadVAO, quadVBO;
	};

	App* CreateApp();
}
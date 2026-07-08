#include "eqnpch.h"
#include "equinox/core/App.h"
#include "equinox/core/Log.h"
#include "equinox/core/Timestep.h"
#include "equinox/window/Window.h"
#include "equinox/input/Input.h"
#include "equinox/editor/Editor.h"
#include "equinox/events/Event.h"
#include "equinox/events/AppEvent.h"
#include "equinox/events/KeyEvent.h"

namespace Equinox
{
	App::App()
	{
		WindowSpec spec;
		spec.Title = "Equinox Engine";

		m_Window = std::make_unique<Window>(spec);
		Input::SetWindow(m_Window->GetNativeWindow());
		Editor::Init(m_Window->GetNativeWindow());
	}

	App::~App()
	{
	}

	void App::Run()
	{
		OnInit();

		while (m_Running)
		{
			f32 time = static_cast<f32>(glfwGetTime());
			f32 dt = time - m_LastFrameTime;
			m_LastFrameTime = time;

			// Update window first
			m_Window->OnUpdate();

			// User-defined update
			OnUpdate(dt);

			// Render UI
			Editor::BeginFrame();
			OnUIRender();
			Editor::EndFrame();
		}
		OnShutdown();
	}

	void App::Close()
	{
		m_Running = false;
	}
}
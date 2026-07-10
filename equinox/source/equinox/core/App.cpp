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
#include "equinox/resources/ShaderLibrary.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/Shader.h"

// TEST need to be removed later
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

namespace Equinox
{
	App::App()
	{
		// Create Window and initialize
		m_Window = Window::Create();
		Input::SetWindow(m_Window->GetHandle());
		Editor::Init(m_Window->GetHandle());
		m_Renderer = Renderer::Create(RendererAPI::Vulkan);
	}

	App::~App()
	{
	}

	void App::Run()
	{
		OnInit();

		while (m_Running)
		{
			f32 time = Timestep::GetTime();
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

			m_Window->SwapBuffers();
		}
		OnShutdown();
	}

	void App::Close()
	{
		m_Running = false;
	}
}
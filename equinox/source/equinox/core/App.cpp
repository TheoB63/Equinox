#include "eqnpch.h"
#include "equinox/core/App.h"

#include "equinox/window/Window.h"
#include "equinox/input/Input.h"
#include "equinox/editor/Editor.h"
#include "equinox/resources/ResourceManager.h"

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
	App::App(int argc, char** argv)
	{
		// Create Window and initialize
		WindowSpec ws = ParseCommandLineArgs(argc, argv);
		ws.VSync = false;

		m_Window = Window::Create(ws);
		Input::SetWindow(m_Window->GetNativeWindow());
		Renderer::Init(ws.rendererAPI, m_Window->GetNativeWindow());
		Editor::Init(m_Window->GetNativeWindow());
		ResourceManager::Init();
	}

	App::~App()
	{
	}

	void App::Run()
	{
		OnInit();

		while (m_Running)
		{
			Time::Update();

			m_Window->OnUpdate();

			OnUpdate();

			if (!m_Window->IsMinimized())
				Renderer::DrawFrame();

			// Render UI
			if (Renderer::GetAPI() == RendererAPI::API::OpenGL)
			{
				Editor::BeginFrame();
				OnUIRender();
				Editor::EndFrame();
			}

			m_Window->SwapBuffers();
			Renderer::Clear();
		}
		OnShutdown();
	}

	void App::Close()
	{
		m_Running = false;
	}

	WindowSpec App::ParseCommandLineArgs(int argc, char** argv)
	{
		WindowSpec spec;
		spec.rendererAPI = RendererAPI::API::OpenGL;

		if (argc < 2) 
		{ // No arguments
			EQN_CORE_WARN("Usage: {} [--vulkan|--rt]", argv[0]);
			EQN_CORE_WARN("Initializing default [--opengl]");
			return spec;
		}

		for (int i = 1; i < argc; ++i) 
		{
			std::string arg = argv[i];
			if (arg == "--opengl")
			{
				spec.rendererAPI = RendererAPI::API::OpenGL;
				break;
			}
			else if (arg == "--vulkan") 
			{
				spec.rendererAPI = RendererAPI::API::Vulkan;
				break;
			}
			else 
			{  // Invalid argument
				EQN_CORE_WARN("Unknown argument: {}", arg);
			}
		}
		return spec;
	}
}
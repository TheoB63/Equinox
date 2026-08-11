#include "eqnpch.h"
#include "equinox/core/App.h"

#include "equinox/window/Window.h"
#include "equinox/input/Input.h"
#include "equinox/events/Event.h"

#include "equinox/resources/FileSystem.h"
#include "equinox/resources/Resources.h"

#include "equinox/editor/Editor.h"
#include "equinox/editor/panels/ScenePanel.h"

#include "equinox/ECS/Systems.h"

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
		FileSystem::Init();
		ResourceDB::Init(FileSystem::AssetsPath());
		Resources::Init();
		Systems::Init();
		Editor::Init(m_Window->GetNativeWindow());

		// Subscribe to events
		EventBus::Subscribe<WindowResizeEvent>(BusType::MainThread, [this](Event& e)
			{
				OnWindowResize(static_cast<WindowResizeEvent&>(e));
			});

		EventBus::Subscribe<WindowCloseEvent>(BusType::MainThread, [this](Event& e)
			{
				OnWindowClose(static_cast<WindowCloseEvent&>(e));
			});

		EventBus::Subscribe<FileDropEvent>(BusType::MainThread, [this](Event& e)
			{
				OnFileDrop(static_cast<FileDropEvent&>(e));
			});
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
			EventBus::ProcessEvents(BusType::MainThread);

			OnUpdate();

			if (!m_Window->IsMinimized())
			{
				Systems::Update<RenderingSystem>();

				// Render UI (not yet implemented in vulkan)
				if (Renderer::GetAPI() == RendererAPI::API::OpenGL)
				{
					Editor::BeginFrame();
					Editor::Render();
					OnUIRender();
					Editor::EndFrame();
				}
			}

			m_Window->SwapBuffers();
			Renderer::Clear();
		}
		OnShutdown();
		Close();
	}

	void App::Close()
	{
		ResourceDB::SaveDirty();
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

	void App::OnWindowResize(WindowResizeEvent& e)
	{

	}

	void App::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
	}

	void App::OnFileDrop(FileDropEvent& e)
	{
		for (const auto& srcPath : e.GetPaths())
		{
			try
			{
				// 1. Validate file
				if (!fs::exists(srcPath))
				{
					EQN_CORE_ERROR("Dropped file not found: {0}", srcPath.string());
					continue;
				}

				// 2. Classify resource type
				ResourceType resType = FileSystem::ClassifyFileType(srcPath);
				if (resType == ResourceType::Unknown) 
				{
					EQN_CORE_WARN("Unsupported file type: {0}", srcPath.string());
					continue;
				}

				// 3. Determine destination path
				fs::path destPath = FileSystem::GetPath(resType, srcPath.stem().string(), true);

				// Create target directory if needed
				FileSystem::CreateDirectories(destPath.parent_path());

				// 4. Copy file to project
				fs::copy_file(srcPath, destPath, fs::copy_options::overwrite_existing);
				EQN_CORE_INFO("Imported {0} to {1}", srcPath.filename().string(), destPath.string());

				// 5. Generate meta file
				UUID newUuid = MetaFile::Create(destPath, resType);

				EQN_CORE_INFO("Created asset {0} with UUID {1}", destPath.filename().string(), newUuid.ToString());
			}
			catch (const fs::filesystem_error& err) 
			{
				EQN_CORE_ERROR("Import failed: {0} - {1}", srcPath.string(), err.what());
			}
			catch (const std::exception& ex) 
			{
				EQN_CORE_ERROR("Asset processing error: {0} - {1}", srcPath.string(), ex.what());
			}
		}
	}
}
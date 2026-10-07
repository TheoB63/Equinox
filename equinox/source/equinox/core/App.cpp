#include "eqnpch.h"
#include "equinox/core/App.h"

#include "equinox/window/Window.h"
#include "equinox/input/Input.h"
#include "equinox/events/Event.h"
#include "equinox/events/AppEvent.h"
#include "equinox/resources/FileSystem.h"
#include "equinox/resources/MetaFile.h"
#include "equinox/editor/Editor.h"
#include "equinox/editor/panels/ScenePanel.h"
#include "equinox/ECS/Systems.h"
#include "equinox/resources/AssetManager.h"
#include "equinox/ECS/systems/TransformSystem.h"
#include "equinox/core/JobSystem.h"
#include "equinox/core/Profiler.h"
#include "equinox/renderer/Renderer.h"

namespace Equinox
{
    App::App(int argc, char** argv)
    {
        // Core Systems Init
        JobSystem::Init();
        FileSystem::Init();
        AssetManager::Init();

        WindowSpec ws = ParseCommandLineArgs(argc, argv);
        SetAppTitle(ws);
        m_Window = Window::Create(ws);
        Input::SetWindow(m_Window->GetNativeWindow());

        // [OP] Dual-backend: initialize the requested renderer API
        // (the window was already created for this API: GL context or
        // context-less for Vulkan).
        Renderer::Init(m_Window->GetNativeWindow(), ws.rendererAPI);
        
        // Scene & Systems
        m_Scene = std::make_shared<Scene>();
        Systems::Init();
        Systems::SetRegistry(m_Scene->RegistryPtr());

        Editor::Init(m_Window.get());

        // Subscribe to events
        EventBus::Subscribe<WindowResizeEvent>(BusType::MainThread, [this](Event& e) {
            OnWindowResize(static_cast<WindowResizeEvent&>(e));
        });

        EventBus::Subscribe<WindowCloseEvent>(BusType::MainThread, [this](Event& e) {
            OnWindowClose(static_cast<WindowCloseEvent&>(e));
        });

        EventBus::Subscribe<FileDropEvent>(BusType::MainThread, [this](Event& e) {
            OnFileDrop(static_cast<FileDropEvent&>(e));
        });
    }

    App::~App() {}

    void App::Run()
    {
        OnInit();

        while (m_Running)
        {
            EQN_PROFILE_FRAME("MainThread");

            Time::Update();
            m_Window->OnUpdate();
            EventBus::ProcessEvents(BusType::MainThread);
            
            OnUpdate();
            AssetManager::Update();

            // Editor Begin
            Editor::BeginFrame();
            Editor::Render(); // Submits ImGui commands to ImGui internal buffers

            // Editor End (Generates DrawData for ImGui)
            Editor::EndFrame();

            // Update and Render additional Platform Windows
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            if (!m_Window->IsMinimized())
            {
                {
                    EQN_PROFILE_SCOPE("Systems::Update");
                    Systems::Update<TransformSystem>();
                    Systems::Update<RenderingSystem>();
                }
            }
        }

        OnShutdown();
        Close();
    }

    void App::Close()
    {
		Editor::Shutdown();
		Systems::Shutdown();
        
        // Renderer::Shutdown(); // Phase 3

        AssetManager::Shutdown();
        JobSystem::Shutdown();
    }

    // [OP] Dual-backend: select the renderer API from the command line.
    //   EquinoxApp.exe --opengl   (or EquinoxApp.exe --vulkan)
    WindowSpec App::ParseCommandLineArgs(int argc, char** argv)
    {
        WindowSpec spec;
        spec.rendererAPI = RendererAPI::API::Vulkan; // Equinox's default API

        if (argc < 2)
        {   // No arguments
            EQN_CORE_WARN("Usage: {0} [--vulkan|--opengl]", argv[0]);
            EQN_CORE_WARN("Initializing default [--vulkan]");
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
            {   // Invalid argument
                EQN_CORE_WARN("Unknown argument: {0}", arg);
            }
        }
        return spec;
    }

    void App::SetAppTitle(WindowSpec& ws)
    {
		std::string title = "Equinox 0.1";

        // [OP] Dual-backend: show the active renderer API in the title
        switch (ws.rendererAPI)
        {
        case RendererAPI::API::OpenGL: title += " [OpenGL]"; break;
        case RendererAPI::API::Vulkan: title += " [Vulkan]"; break;
        default: title += " [Unknown API]"; break;
        }

#ifdef _WIN32
		title += " - Windows";
#endif

		ws.Title = title;
    }

    void App::OnWindowResize(WindowResizeEvent& e)
    {
        if (e.GetWidth() == 0 || e.GetHeight() == 0)
            return;

        Renderer::OnResize(e.GetWidth(), e.GetHeight());
    }

    void App::OnWindowClose(WindowCloseEvent& e)
    {
        m_Running = false;
    }

    void App::OnFileDrop(FileDropEvent& e)
    {
        for (const auto& srcPath : e.GetPaths()) {
            try {
                // 1. Validate file
                if (!fs::exists(srcPath)) {
                    EQN_CORE_ERROR("Dropped file not found: {0}", srcPath.string());
                    continue;
                }

                // 2. Classify asset type
                AssetType resType = FileSystem::ClassifyFileType(srcPath);
                if (resType == AssetType::None) {
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
            catch (const fs::filesystem_error& err) {
                EQN_CORE_ERROR("Import failed: {0} - {1}", srcPath.string(), err.what());
            }
            catch (const std::exception& ex) {
                EQN_CORE_ERROR("Asset processing error: {0} - {1}", srcPath.string(), ex.what());
            }
        }
    }
}

#include "eqnpch.h"
#include "equinox/core/App.h"

#include "equinox/platform/Window.h"
#include "equinox/platform/Input.h"
#include "equinox/platform/Event.h"
#include "equinox/platform/AppEvent.h"

#include "equinox/resources/FileSystem.h"
#include "equinox/resources/MetaFile.h"
#include "equinox/resources/AssetManager.h"
#include "equinox/resources/AssetDatabase.h"

#include "equinox/editor/Editor.h"
#include "equinox/editor/panels/ScenePanel.h"

#include "equinox/scene/Systems.h"
#include "equinox/scene/systems/TransformSystem.h"
#include "equinox/scene/systems/RenderingSystem.h"

#include "equinox/core/Profiler.h"

#include "equinox/jobs/JobSystem.h"
#include "equinox/jobs/JobSystemTests.h"
#include "equinox/jobs/IOThread.h"

#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/ShaderLibrary.h"

namespace Equinox
{
	App::App(int argc, char** argv)
	{
		// Core Systems Init
		JobSystem::Init();
		IOThread::Init();
		m_FrameData.Init();

		// Run Job System Verification Tests
		//JobSystem::Tests::RunAll();

		FileSystem::Init();
		AssetDatabase::Init(FileSystem::AssetsPath());
		AssetManager::Init();

		// Import Phase: Process any stale assets found by AssetDatabase
		const auto& dirtyAssetsRef = AssetDatabase::GetDirtyAssets();
		if (!dirtyAssetsRef.empty())
		{
			std::vector<UUID> assetsToImport = dirtyAssetsRef;

			EQN_CORE_INFO("Importing {0} assets in parallel...", assetsToImport.size());

			JobSystem::Counter importCounter(0);

			JobSystem::Dispatch((u32)assetsToImport.size(), 1, [](JobSystem::JobArgs args) {
				std::vector<UUID>* assets = (std::vector<UUID>*)args.data;
				AssetManager::Import((*assets)[args.jobIndex]);
				}, &assetsToImport, &importCounter);

			JobSystem::WaitForCounter(&importCounter);
		}

		WindowSpec ws = ParseCommandLineArgs(argc, argv);
		SetAppTitle(ws);
		m_Window = Window::Create(ws);
		Input::Init();

		// [OP] Dual-backend: initialize the requested renderer API
		// (the window was already created for this API: GL context or
		// context-less for Vulkan).
		Renderer::Init(m_Window->GetNativeWindow());
		Renderer::SetFrameData(&m_FrameData);
		ShaderLibrary::Init();

		// Scene & Systems
		m_Scene = std::make_shared<Scene>();
		Systems::Init();
		Systems::SetScene(m_Scene.get());

		Editor::Init(m_Window.get());

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

	// ===============================================================================
	// Pipelined Engine Loop (Main Thread Isolated)
	// ===============================================================================
	// Structure (target):
	//   1. glfwPollEvents()			— OS message pump (main-thread-only)
	//   2. TryReclaimGPU(N-2)			— Non-blocking GPU completion check
	//   3. KickGame(Frame[N])			— Dispatch game logic to workers
	//   4. WaitForCounter(GameReady)	— Main thread busy-spins (isolated)
	//   5. KickRender(Frame[N-1])		— Dispatch render recording to workers
	//   6. WaitForCounter(RenderReady) — Wait for recording
	//   7. Submit(Frame[N-1])			— Send command buffers to GPU
	//   8. Present()					— Swapchain present (main-thread-only)
	//
	// Phase 2 Implementation: Frame data flow is correct. Full job parallelism
	// between Game(N) and Render(N-1) is wired structurally but runs sequentially
	// within each frame until Phase 3 (Render Graph) enables proper parallel recording.

	void App::Run()
	{
		OnInit();

		while (m_Running)
		{
			EQN_PROFILE_FRAME("MainThread");

			u64 frameIndex = m_FrameData.GetFrameIndex();

			// 1. OS Message Pump (Main thread only)

			Time::Update();
			m_Window->OnUpdate();
			EventBus::ProcessEvents(BusType::MainThread);

			if (m_Window->IsMinimized())
			{
				std::this_thread::yield();
				continue;
			}

			// 2. GPU Reclaim (N-2)
			// For the first few frames, there's nothing to reclaim
			FrameContext& currentFrame = m_FrameData.Current();
			if (frameIndex >= MAX_FRAMES_IN_FLIGHT)
			{
				FrameContext& gpuFrame = m_FrameData.GPU();
				// Check if GPU(N-2) is done. If not, current frame uses overflow.
				// For now, the blocking wait in AcquireImage handles this.
				// When PollerJobs are wired (Phase 3+), this becomes non-blocking.
			}

			// 3. Begin Vulkan Frame
			// Acquires swapchain image, waits on timeline for this slot's previous use
			Renderer::BeginFrame(frameIndex);

			// Reset frame resources now that GPU is done with this slot
			currentFrame.Reset();
			currentFrame.Params.DeltaTime = Time::DeltaTime();
			currentFrame.Params.TotalTime = Time::GetTime();
			currentFrame.Params.FrameNumber = frameIndex;

			// 4. Game Logic
			Editor::BeginFrame();
			OnUpdate();
			AssetManager::Update();
			Editor::Render();
			Editor::EndFrame();

			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}

			Systems::Update<TransformSystem>();
			Systems::Update<RenderingSystem>();

			// 5. End Vulkan Frame (Submit + Present)
			Renderer::EndFrame();

			// 6. Advence Frame Data
			m_FrameData.Advance();
			JobSystem::ResetFrameStats();
		}

		OnShutdown();
		Close();
	}

	void App::Close()
	{
		Editor::Shutdown();
		Systems::Shutdown();

		AssetManager::Shutdown();
		AssetDatabase::Shutdown();
		ShaderLibrary::Shutdown();
		Renderer::Shutdown();
		m_FrameData.Shutdown();
		IOThread::Shutdown();
		JobSystem::Shutdown();
	}

	WindowSpec App::ParseCommandLineArgs(int argc, char** argv)
	{
		WindowSpec spec;
		spec.rendererAPI = RenderBackend::API::Vulkan; // Equinox's default API

		if (argc < 2)
		{
			// No arguments
			EQN_CORE_WARN("Usage: {0} [--vulkan|--opengl]", argv[0]);
			EQN_CORE_WARN("Initializing default [--vulkan]");
			return spec;
		}

		for (int i = 1; i < argc; ++i)
		{
			std::string arg = argv[i];
			if (arg == "--vulkan")
			{
				spec.rendererAPI = RenderBackend::API::Vulkan;
				break;
			}
			else
			{
				// Invalid argument
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
		case RenderBackend::API::Vulkan: title += " [Vulkan]";
			break;
		default: title += " [Unknown API]";
			break;
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

				// 2. Classify asset type
				AssetType resType = FileSystem::ClassifyFileType(srcPath);
				if (resType == AssetType::None)
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

#include <Equinox.h>
#include <Equinox/core/EntryPoint.h>

#include <equinox/graphics/GfxRenderer.h>
#include <equinox/graphics/GfxScene.h>

#include <imgui.h>

namespace Equinox
{
	class EquinoxApp : public App
	{
	public:
		EquinoxApp(int argc, char** argv) : App(argc, argv) {}
		~EquinoxApp() override = default;

	protected:
		void OnInit() override {}

		void OnUpdate() override
		{
		}

		void OnUIRender() override
		{
			// ImGui Demo Window
			static bool showDemo = true;
			if (showDemo) ImGui::ShowDemoWindow(&showDemo);

			ImGuiIO& io = ImGui::GetIO();
			Gfx::FrameStats& stats = Gfx::GfxRenderer::GetStats();
			const Gfx::GPUInfo& gpu = Gfx::GfxRenderer::GetGPUInfo();
			const Gfx::SceneTargetSize target = Gfx::GfxScene::GetTargetSize();

			ImGui::Begin("Equinox Metrics");

			ImGui::SeparatorText("Frame");
			const float fps = io.Framerate > 0.0f ? io.Framerate : 0.0f;
			ImGui::Text("Frame time : %.2f ms  (%.1f FPS)", fps > 0.0f ? 1000.0f / fps : 0.0f, fps);
			if (stats.cpuFrameMs > 0.0f)
				ImGui::Text("CPU frame  : %.2f ms", stats.cpuFrameMs);
			if (gpu.timestamps)
				ImGui::Text("GPU frame  : %.2f ms", stats.gpuFrameMs);
			else
				ImGui::TextDisabled("GPU frame  : timestamps not available");

			ImGui::SeparatorText("Scene");
			if (target.width > 0 && target.height > 0)
				ImGui::Text("Target         : %u x %u", target.width, target.height);
			ImGui::Text("Draw calls    : %u", stats.drawCalls);
			ImGui::Text("Objets        : %u visible / %u", stats.objectsVisible, stats.objectsTotal);
			ImGui::Text("Preparation   : %.4f ms", stats.scenePrepMs);

			ImGui::SeparatorText("Setup the scene (unified path)");
			{
				const Gfx::ScenePrepareStats& prep = Gfx::GfxScene::GetPrepareStats();
				if (prep.measuredAB)
				{
					ImGui::Text("1 thread  : %.4f ms", prep.singleThreadMs);
					ImGui::Text("JobSystem : %.4f ms  (%u threads)", prep.jobsMs, prep.threadCount);
					ImGui::TextDisabled("Use : %s", prep.jobsUsed ? "JobSystem" : "1 thread");
				}
				else
				{
					ImGui::TextDisabled("A/B not measured (more than 2048 objects)");
				}
			}

			ImGui::SeparatorText("API");
			ImGui::Text("%s", Gfx::GfxRenderer::GetBackendName());
			if (!gpu.device.empty())
				ImGui::TextUnformatted(gpu.device.c_str());
			if (!gpu.apiVersion.empty())
				ImGui::TextDisabled("%s", gpu.apiVersion.c_str());
			if (!gpu.driver.empty())
				ImGui::TextDisabled("%s", gpu.driver.c_str());

			ImGui::SeparatorText("Vsync");
			static bool vsync = true;
			if (ImGui::Checkbox("Vsync##EquinoxMetrics", &vsync))
				Gfx::GfxRenderer::SetVSync(vsync);

			ImGui::End();
		}

		void OnShutdown() override {}
	};

	App* CreateApp(int argc, char** argv)
	{
		return new EquinoxApp(argc,argv);
	}
}
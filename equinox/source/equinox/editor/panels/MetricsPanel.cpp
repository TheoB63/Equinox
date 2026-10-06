#include "eqnpch.h"
#include "equinox/editor/panels/MetricsPanel.h"

#include "equinox/graphics/GfxRenderer.h"
#include "equinox/graphics/GfxScene.h"

#include <imgui.h>

namespace Equinox
{
	void MetricsPanel::OnInit()
	{
	}

	void MetricsPanel::OnRender()
	{
		ImGuiIO& io = ImGui::GetIO();
		Gfx::FrameStats& stats = Gfx::GfxRenderer::GetStats();
		const Gfx::GPUInfo& gpu = Gfx::GfxRenderer::GetGPUInfo();
		const Gfx::SceneTargetSize target = Gfx::GfxScene::GetTargetSize();
		const Gfx::SceneTargetSize surface = Gfx::GfxRenderer::GetSwapchainExtent();
		const Gfx::ScenePrepareStats& prep = Gfx::GfxScene::GetPrepareStats();

		ImGui::Begin("Equinox Metrics");

		ImGui::SeparatorText("Frame");
		const float fps = io.Framerate > 0.0f ? io.Framerate : 0.0f;
		ImGui::Text("Frame time : %.2f ms  (%.1f FPS)", fps > 0.0f ? 1000.0f / fps : 0.0f, fps);
		ImGui::Text("CPU frame  : %.2f ms", stats.cpuFrameMs);
		if (gpu.timestamps)
			ImGui::Text("GPU frame  : %.2f ms", stats.gpuFrameMs);
		else
			ImGui::TextDisabled("GPU frame  : timestamps unavailable");

		ImGui::SeparatorText("Scene");
		if (target.width > 0 && target.height > 0)
			ImGui::Text("Render target     : %u x %u", target.width, target.height);
		ImGui::Text("Objects           : %u visible / %u", stats.objectsVisible, stats.objectsTotal);
		ImGui::Text("Opaque / blended  : %u / %u", stats.opaqueObjects, stats.transparentObjects);
		ImGui::Text("Scene preparation : %.4f ms", stats.scenePrepMs);

		ImGui::SeparatorText("Rendering work");
		ImGui::Text("Draw calls total  : %u", stats.drawCalls);
		ImGui::Text("Geometry draws    : %u", stats.geometryDrawCalls);
		ImGui::Text("Transparent draws : %u", stats.transparentDrawCalls);
		ImGui::Text("Fullscreen passes : %u", stats.fullscreenPasses);
		ImGui::Text("Bloom passes      : %u", stats.bloomPasses);
		ImGui::Text("Cached meshes     : %u", stats.cachedMeshes);
		ImGui::Text("Cached textures   : %u", stats.cachedTextures);

		ImGui::SeparatorText("Scene preparation detail");
		ImGui::Text("Items             : %u (%u visible)", prep.items, prep.visible);
		ImGui::Text("Workers           : %u", prep.threadCount);
		ImGui::Text("Break-even        : %u items", prep.breakEvenItems);
		if (prep.measuredAB)
		{
			ImGui::Text("1 thread          : %.4f ms", prep.singleThreadMs);
			ImGui::Text("Job System        : %.4f ms", prep.jobsMs);
			ImGui::TextDisabled("Selected: %s", prep.jobsUsed ? "Job System" : "1 thread");
		}
		else
		{
			ImGui::TextDisabled("A/B timing skipped above 2048 objects");
		}

		ImGui::SeparatorText("Graphics API");
		ImGui::Text("Backend           : %s", Gfx::GfxRenderer::GetBackendName());
		if (!gpu.device.empty()) ImGui::Text("Device            : %s", gpu.device.c_str());
		if (!gpu.apiVersion.empty()) ImGui::Text("API version       : %s", gpu.apiVersion.c_str());
		if (!gpu.driver.empty()) ImGui::TextWrapped("Driver            : %s", gpu.driver.c_str());
		ImGui::Text("Max anisotropy    : %.1fx", gpu.maxAnisotropy);
		ImGui::Text("GPU timestamps    : %s", gpu.timestamps ? "yes" : "no");
		ImGui::Text("Multi-viewport    : %s", gpu.multiViewport ? "yes" : "no");

		ImGui::SeparatorText("Presentation");
		if (surface.width > 0 && surface.height > 0)
			ImGui::Text("Surface extent    : %u x %u", surface.width, surface.height);
		ImGui::Text("Swapchain images  : %u", Gfx::GfxRenderer::GetSwapchainImageCount());
		ImGui::Text("Surface format    : 0x%llX",
			static_cast<unsigned long long>(Gfx::GfxRenderer::GetSwapchainFormat()));
		if (ImGui::Checkbox("VSync##EquinoxMetrics", &m_VSync))
			Gfx::GfxRenderer::SetVSync(m_VSync);

		ImGui::End();
	}
}

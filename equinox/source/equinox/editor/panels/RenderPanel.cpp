#include "eqnpch.h"
#include "equinox/editor/panels/RenderPanel.h"
#include "equinox/resources/libraries/ShaderLibrary.h"
#include "equinox/ECS/Systems.h"
#include "equinox/ECS/Systems/RenderingSystem.h"
#include "equinox/utils/EquinoxIcons.h"

#include "equinox/graphics/GfxRenderer.h"
#include "equinox/graphics/GfxScene.h"

namespace Equinox
{
	RenderPanel::RenderPanel() : m_SelectedMode("Final")
	{
		EQN_CORE_INFO("Created Render panel");
	}

	void RenderPanel::OnInit()
	{
		m_RS = Systems::GetSystem<RenderingSystem>();

		if (m_RS && m_RS->GetActivePipeline())
		{
			auto* p = m_RS->GetActivePipeline();
			//m_SelectedAttachment = p->GetFinalColorAttachment();
			m_SelectedAttachment = p->GetPass<PostProcessPass>()->GetGBuffer()->GetColorAttachmentID(0);
		}
	}

	bool RenderPanel::IsVulkan() const
	{
		return Gfx::GfxRenderer::GetBackendType() == Gfx::BackendType::Vulkan;
	}

	bool RenderPanel::UsesPipeline() const
	{
		return m_RS && !IsVulkan() && m_RS->GetActivePipeline() != nullptr;
	}

	void RenderPanel::OnRender()
	{
		if (!m_RS) return;
		if (!IsVulkan() && !m_RS->GetActivePipeline()) return;

		ImGui::PushFont(Editor::GetFASolid());
		std::string render = ICON_FA_FILM + std::string("  Render");
		ImGui::Begin(render.c_str());

		// Tab selector
		if (ImGui::Button(ICON_FA_LAYER_GROUP, ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0)))
		{
			m_SelectedTab = 0;
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_SLIDERS, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
		{
			m_SelectedTab = 1;
		}
		ImGui::PopFont();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Show the selected content
		if (m_SelectedTab == 0)
		{
			// Model Viewer ==========================
			for (auto& [title, modes] : m_Groups)
			{
				DrawGroup(title, modes);
				ImGui::Dummy({ 0, 4 });
			}
		}
		else
		{
			if (IsVulkan())
				DrawPostProcessVulkan();
			else
				DrawPostProcessOpenGL();
		}
		ImGui::End();
	}

	void RenderPanel::DrawPostProcessOpenGL()
	{
		auto* p = m_RS->GetActivePipeline();
		auto g_SSAOPass = p->GetPass<SSAOPass>();
		auto g_PostProcessPass = p->GetPass<PostProcessPass>();

		// SSAO
		if (ImGui::CollapsingHeader("SSAO", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float ssaoRadius = g_SSAOPass->GetRadius();
			if (ImGui::SliderFloat("Radius", &ssaoRadius, 0.0f, 10.0f))
			{
				g_SSAOPass->SetRadius(ssaoRadius);
			}

			float ssaoIntensity = g_SSAOPass->GetIntensity();
			if (ImGui::SliderFloat("Intensity", &ssaoIntensity, 0.0f, 1.0f))
			{
				g_SSAOPass->SetIntensity(ssaoIntensity);
			}

			float ssaoBias = g_SSAOPass->GetBias();
			if (ImGui::SliderFloat("Bias", &ssaoBias, 0.05f, 0.5f))
			{
				g_SSAOPass->SetBias(ssaoBias);
			}
		}

		// Bloom
		if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float bloomThreshold = g_PostProcessPass->GetBloomThreshold();
			if (ImGui::SliderFloat("Threshold", &bloomThreshold, 0.0f, 2.0f))
			{
				g_PostProcessPass->SetBloomThreshold(bloomThreshold);
			}

			float bloomStrength = g_PostProcessPass->GetBloomStrength();
			if (ImGui::SliderFloat("Strength", &bloomStrength, 0.0f, 2.0f))
			{
				g_PostProcessPass->SetBloomStrength(bloomStrength);
			}

			int bloomPasses = g_PostProcessPass->GetBloomPasses();
			if (ImGui::SliderInt("Passes", &bloomPasses, 1, 10))
			{
				g_PostProcessPass->SetBloomPasses(bloomPasses);
			}
		}

		// Tone Mapping
		if (ImGui::CollapsingHeader("Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const char* operators[] =
			{
				"Linear", "Reinhard", "Modified Reinhard",
				"ACES", "Filmic", "Uncharted 2"
			};

			int currentOp = static_cast<int>(g_PostProcessPass->GetToneMapOperator());
			if (ImGui::Combo("Operator", &currentOp, operators, IM_ARRAYSIZE(operators)))
			{
				g_PostProcessPass->SetToneMapOperator(static_cast<ToneMapOperator>(currentOp));
			}

			// Exposure control
			float exposure = g_PostProcessPass->GetExposure();
			if (ImGui::SliderFloat("Exposure", &exposure, 0.1f, 5.0f, "%.2f"))
			{
				g_PostProcessPass->SetExposure(exposure);
			}

			// Contrast control
			float contrast = g_PostProcessPass->GetContrast();
			if (ImGui::SliderFloat("Contrast", &contrast, 0.5f, 2.0f, "%.2f"))
			{
				g_PostProcessPass->SetContrast(contrast);
			}

			// Saturation control
			float saturation = g_PostProcessPass->GetSaturation();
			if (ImGui::SliderFloat("Saturation", &saturation, 0.0f, 2.0f, "%.2f"))
			{
				g_PostProcessPass->SetSaturation(saturation);
			}
		}

		// Color Balance
		if (ImGui::CollapsingHeader("Color Balance", ImGuiTreeNodeFlags_DefaultOpen))
		{
			glm::vec3 shadows = g_PostProcessPass->GetShadowBalance();
			if (ImGui::ColorEdit3("Shadows", glm::value_ptr(shadows)))
			{
				g_PostProcessPass->SetColorBalance(
					shadows,
					g_PostProcessPass->GetMidtoneBalance(),
					g_PostProcessPass->GetHighlightBalance()
				);
			}

			glm::vec3 midtones = g_PostProcessPass->GetMidtoneBalance();
			if (ImGui::ColorEdit3("Midtones", glm::value_ptr(midtones)))
			{
				g_PostProcessPass->SetColorBalance(
					g_PostProcessPass->GetShadowBalance(),
					midtones,
					g_PostProcessPass->GetHighlightBalance()
				);
			}

			glm::vec3 highlights = g_PostProcessPass->GetHighlightBalance();
			if (ImGui::ColorEdit3("Highlights", glm::value_ptr(highlights)))
			{
				g_PostProcessPass->SetColorBalance(
					g_PostProcessPass->GetShadowBalance(),
					g_PostProcessPass->GetMidtoneBalance(),
					highlights
				);
			}
		}

		// Vignette
		if (ImGui::CollapsingHeader("Vignette", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float vignetteAmount = g_PostProcessPass->GetVignetteAmount();
			if (ImGui::SliderFloat("Amount", &vignetteAmount, 0.0f, 1.0f))
			{
				g_PostProcessPass->SetVignetteAmount(vignetteAmount);
			}

			float vignetteHardness = g_PostProcessPass->GetVignetteHardness();
			if (ImGui::SliderFloat("Hardness", &vignetteHardness, 0.0f, 1.0f))
			{
				g_PostProcessPass->SetVignetteHardness(vignetteHardness);
			}
		}

		// Others
		if (ImGui::CollapsingHeader("Others", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// Grain
			float grainAmount = g_PostProcessPass->GetGrainAmount();
			if (ImGui::SliderFloat("Grain Amount", &grainAmount, 0.0f, 0.2f, "%.3f"))
			{
				g_PostProcessPass->SetGrainAmount(grainAmount);
			}

			// Sharpness
			float sharpness = g_PostProcessPass->GetSharpness();
			if (ImGui::SliderFloat("Sharpness", &sharpness, -1.0f, 1.0f))
			{
				g_PostProcessPass->SetSharpness(sharpness);
			}

			// Chromatic Aberration
			float aberrationOffset = g_PostProcessPass->GetAberrationOffset();
			if (ImGui::SliderFloat("Chromatic Aberration", &aberrationOffset, 0.0f, 0.02f, "%.4f"))
			{
				g_PostProcessPass->SetAberrationOffset(aberrationOffset);
			}
		}
	}

	void RenderPanel::DrawPostProcessVulkan()
	{
		Gfx::SceneEffects& fx = Gfx::GfxScene::GetSettings().effects;

		const char* kOpenGLOnly = "Requires a separate pass (G-buffer / intermediate target) :\n"
		                          "available in OpenGL, but not in Vulkan direct rendering.";

		// SSAO ---------------------------------------------------------------
		if (ImGui::CollapsingHeader("SSAO", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::BeginDisabled();
			ImGui::SliderFloat("Radius", &fx.ssaoRadius, 0.0f, 10.0f);
			ImGui::SliderFloat("Intensity", &fx.ssaoIntensity, 0.0f, 1.0f);
			ImGui::SliderFloat("Bias", &fx.ssaoBias, 0.05f, 0.5f);
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				ImGui::SetTooltip("%s", kOpenGLOnly);
		}

		// Bloom --------------------------------------------------------------
		if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::BeginDisabled();
			ImGui::SliderFloat("Threshold", &fx.bloomThreshold, 0.0f, 2.0f);
			ImGui::SliderFloat("Strength", &fx.bloomStrength, 0.0f, 2.0f);
			ImGui::SliderInt("Passes", &fx.bloomPasses, 1, 10);
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				ImGui::SetTooltip("%s", kOpenGLOnly);
		}

		// Tone Mapping -------------------------------------------------------
		if (ImGui::CollapsingHeader("Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const char* operators[] =
			{
				"Linear", "Reinhard", "Modified Reinhard",
				"ACES", "Filmic", "Uncharted 2"
			};

			int currentOp = fx.toneMapping;
			if (ImGui::Combo("Operator", &currentOp, operators, IM_ARRAYSIZE(operators)))
			{
				fx.toneMapping = currentOp;
			}

			ImGui::SliderFloat("Exposure", &fx.exposure, 0.1f, 5.0f, "%.2f");
			ImGui::SliderFloat("Contrast", &fx.contrast, 0.5f, 2.0f, "%.2f");
			ImGui::SliderFloat("Saturation", &fx.saturation, 0.0f, 2.0f, "%.2f");
		}

		// Color Balance ------------------------------------------------------
		if (ImGui::CollapsingHeader("Color Balance", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::ColorEdit3("Shadows", glm::value_ptr(fx.shadowBalance));
			ImGui::ColorEdit3("Midtones", glm::value_ptr(fx.midtoneBalance));
			ImGui::ColorEdit3("Highlights", glm::value_ptr(fx.highlightBalance));
		}

		// Vignette -----------------------------------------------------------
		if (ImGui::CollapsingHeader("Vignette", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("Amount", &fx.vignetteAmount, 0.0f, 1.0f);
			ImGui::SliderFloat("Hardness", &fx.vignetteHardness, 0.0f, 1.0f);
		}

		// Others -------------------------------------------------------------
		if (ImGui::CollapsingHeader("Others", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("Grain Amount", &fx.grainAmount, 0.0f, 0.2f, "%.3f");

			ImGui::BeginDisabled();
			ImGui::SliderFloat("Sharpness", &fx.sharpness, -1.0f, 1.0f);
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				ImGui::SetTooltip("%s", kOpenGLOnly);

			ImGui::SliderFloat("Chromatic Aberration", &fx.aberrationOffset, 0.0f, 0.02f, "%.4f");
		}

		// Scene settings (Vulkan) -------------------------------------------
		DrawVulkanSceneSettings();
	}

	void RenderPanel::DrawVulkanSceneSettings()
	{
		Gfx::SceneSettings& s = Gfx::GfxScene::GetSettings();

		ImGui::Spacing();
		if (!ImGui::CollapsingHeader("Scene (Vulkan)"))
			return;

		// ---- lumieres de la hierarchie ------------------------------------
		ImGui::Checkbox("Use ECS lights", &s.useEcsLights);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("<DirectionalLight> / <PointLight> in the hierarchie.\n"
				"Rotation of the entitie = direction (-Z), position = position.");

		ImGui::ColorEdit3("Light Color", glm::value_ptr(s.lightColor));
		ImGui::DragFloat3("Direction", glm::value_ptr(s.lightDirection), 0.01f, -1.0f, 1.0f);
		ImGui::SliderFloat("Ambient", &s.ambient, 0.0f, 1.0f);

		// ---- fond ----------------------------------------------------------
		ImGui::ColorEdit3("Clear Color", glm::value_ptr(s.clearColor));

		// ---- preparation de la scene --------------------------------------
		const char* modes[] = { "1 thread", "Job System", "Auto" };
		int mode = (int)s.jobMode;
		if (ImGui::Combo("Scene Prepare", &mode, modes, IM_ARRAYSIZE(modes)))
			s.jobMode = (Gfx::SceneJobMode)mode;
		if (ImGui::IsItemHovered())
		{
			const Gfx::ScenePrepareStats& prep = Gfx::GfxScene::GetPrepareStats();
			ImGui::SetTooltip("Prepare (matrices + culling + lumieres) :\n"
				"1 thread  : %.4f ms\n"
				"JobSystem : %.4f ms (%u threads)\n"
				"Utilise   : %s", prep.singleThreadMs, prep.jobsMs, prep.threadCount,
				prep.jobsUsed ? "JobSystem" : "1 thread");
		}

		ImGui::Checkbox("Culling", &s.culling);
	}

	void RenderPanel::DrawGroup(const char* title, const std::vector<const char*>& modes)
	{
		ImGui::TextUnformatted(title);
		ImGui::Spacing();

		const bool vulkan = IsVulkan();

		// grab pipeline here
		auto* pipeline = m_RS->GetActivePipeline();
		if (!vulkan && !pipeline) return;

		if (vulkan)
			ImGui::TextDisabled("(canaux du G-buffer : OpenGL seulement)");

		for (auto mode : modes)
		{
			const bool available = !vulkan
				|| std::string(mode) == "Final"
				|| std::string(mode) == "No Post-Processing";

			bool isSelected = (mode == m_SelectedMode);

			if (!available)
				ImGui::BeginDisabled();

			// highlight selected
			if (isSelected && available)
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

			if (ImGui::Button(mode, ImVec2(120, 0)) && available)
			{
				m_SelectedMode = mode;

				if (vulkan)
					Gfx::GfxScene::GetSettings().effects.enabled = (std::string(mode) != "No Post-Processing");
				else
					m_SelectedAttachment = pipeline->GetAttachmentByName(m_SelectedMode);
			}

			if (!available && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				ImGui::SetTooltip("Canal du G-buffer du pipeline differe : disponible en OpenGL seulement.");

			if (isSelected && available)
				ImGui::PopStyleColor();

			if (!available)
				ImGui::EndDisabled();
		}
	}

	void RenderPanel::ApplyRenderingSettings()
	{
		//glPolygonMode(GL_FRONT_AND_BACK, m_Wireframe ? GL_LINE : GL_FILL);
		//m_SingleSided ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
		//m_DepthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
		//glClearColor(m_ClearColor.x, m_ClearColor.y, m_ClearColor.z, m_ClearColor.w);
	}
}
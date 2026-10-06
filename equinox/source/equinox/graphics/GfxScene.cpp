#include "eqnpch.h"
#include "equinox/graphics/GfxScene.h"
#include "equinox/graphics/GfxRenderer.h"

#include "equinox/core/Time.h"
#include "equinox/core/JobSystem.h"
#include "equinox/core/Log.h"
#include "equinox/core/Profiler.h"

#include "equinox/ECS/Components.h"
#include "equinox/renderer/Buffer.h"
#include "equinox/renderer/Mesh.h"
#include "equinox/renderer/Model.h"
#include "equinox/renderer/Texture.h"
#include "equinox/resources/libraries/MaterialLibrary.h"
#include "equinox/resources/libraries/ModelLibrary.h"
#include "equinox/resources/libraries/TextureCache.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>

namespace Equinox::Gfx
{
	namespace
	{
		constexpr u32 MaxDirLights = 4;
		constexpr u32 MaxPointLights = 16;

		struct DirLightCPU
		{
			Vec3  direction{ 0.0f, -1.0f, 0.0f };
			Vec3  color{ 1.0f };
			float intensity = 1.0f;
		};

		struct PointLightCPU
		{
			Vec3  position{ 0.0f };
			Vec3  color{ 1.0f };
			float intensity = 1.0f;
			float range = 350.0f;
		};

		struct LightSet
		{
			DirLightCPU   dir[MaxDirLights];
			u32           dirCount = 0;
			PointLightCPU point[MaxPointLights];
			u32           pointCount = 0;

			u32 Total() const { return dirCount + pointCount; }
		};

		struct EntityItem
		{
			const Mesh* mesh = nullptr;
			const Texture* texture = nullptr;
			Mat4  model{ 1.0f };
			Vec3  center{ 0.0f };
			float radius = 1.0f;
			Vec4  color{ 1.0f };
			float roughness = 0.5f;
			float metallic = 0.0f;
			Vec4  emissive{ 0.0f };
		};

		struct State
		{
			bool initialized = false;
			SceneSettings settings;
			ScenePrepareStats prepStats;

			std::shared_ptr<Mesh> fallbackCube;

			std::vector<EntityItem> prepared;
			std::vector<DrawItem>   results;
			std::vector<u8>         visibleFlags;
			std::vector<Vec4>       frustum;

			std::unordered_map<const Mesh*, float> meshRadius;

			// Times are smoothed to stay readable in the panel.
			double singleThreadMs = 0.0;
			double jobsMs = 0.0;
			bool   loggedFirstFrame = false;
		};

		State s;

		static double NowMs()
		{
			using namespace std::chrono;
			return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
		}

		static double Smooth(double previous, double measured)
		{
			return previous <= 0.0 ? measured : previous * 0.9 + measured * 0.1;
		}

		std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices, const std::vector<u32>& indices)
		{
			auto vb = VertexBuffer::Create(vertices.data(), (u32)(vertices.size() * sizeof(Vertex)));

			vb->SetLayout({
				{ ShaderDataType::Float3, "a_Position"  },
				{ ShaderDataType::Float3, "a_Normal"    },
				{ ShaderDataType::Float2, "a_TexCoord0" },
				{ ShaderDataType::Float2, "a_TexCoord1" },
				{ ShaderDataType::Float3, "a_Tangent"   } });

			auto ib = IndexBuffer::Create(indices.data(), (u32)indices.size());
			return Mesh::Create(vb, ib);
		}

		std::shared_ptr<Mesh> CreateCubeMesh()
		{
			const Vec3 n[6] = {
				{ 0, 0, 1 }, { 0, 0, -1 }, { 1, 0, 0 },
				{ -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }
			};
			const Vec3 u[6] = {
				{ 1, 0, 0 }, { -1, 0, 0 }, { 0, 0, -1 },
				{ 0, 0, 1 }, { 1, 0, 0 }, { 1, 0, 0 }
			};
			const Vec3 v[6] = {
				{ 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 },
				{ 0, 1, 0 }, { 0, 0, -1 }, { 0, 0, 1 }
			};

			std::vector<Vertex> vertices;
			std::vector<u32> indices;
			vertices.reserve(24);

			for (int f = 0; f < 6; f++)
			{
				const Vec3 c = n[f] * 0.5f;
				const u32 base = (u32)vertices.size();

				Vertex vl, vr, tr, tl;
				vl.Position = c - u[f] * 0.5f - v[f] * 0.5f;
				vr.Position = c + u[f] * 0.5f - v[f] * 0.5f;
				tr.Position = c + u[f] * 0.5f + v[f] * 0.5f;
				tl.Position = c - u[f] * 0.5f + v[f] * 0.5f;

				vl.Normal = vr.Normal = tr.Normal = tl.Normal = n[f];
				vl.TexCoord0 = { 0.0f, 0.0f };
				vr.TexCoord0 = { 1.0f, 0.0f };
				tr.TexCoord0 = { 1.0f, 1.0f };
				tl.TexCoord0 = { 0.0f, 1.0f };

				vertices.push_back(vl);
				vertices.push_back(vr);
				vertices.push_back(tr);
				vertices.push_back(tl);

				indices.insert(indices.end(), { base + 0, base + 1, base + 2, base + 0, base + 2, base + 3 });
			}

			return CreateMesh(vertices, indices);
		}

		void ExtractFrustum(const Mat4& viewProj)
		{
			s.frustum.resize(6);

			const Vec4 row0(viewProj[0][0], viewProj[1][0], viewProj[2][0], viewProj[3][0]);
			const Vec4 row1(viewProj[0][1], viewProj[1][1], viewProj[2][1], viewProj[3][1]);
			const Vec4 row2(viewProj[0][2], viewProj[1][2], viewProj[2][2], viewProj[3][2]);
			const Vec4 row3(viewProj[0][3], viewProj[1][3], viewProj[2][3], viewProj[3][3]);

			s.frustum[0] = row3 + row0;   // left
			s.frustum[1] = row3 - row0;   // right
			s.frustum[2] = row3 + row1;   // bottom
			s.frustum[3] = row3 - row1;   // top
			s.frustum[4] = row2;          // near (depth 0..1)
			s.frustum[5] = row3 - row2;   // far

			for (Vec4& plane : s.frustum)
			{
				const float length = glm::length(Vec3(plane));
				if (length > 0.00001f) plane /= length;
			}
		}

		bool SphereInsideFrustum(const Vec3& center, float radius)
		{
			for (const Vec4& plane : s.frustum)
			{
				if (glm::dot(Vec3(plane), center) + plane.w < -radius)
					return false;
			}
			return true;
		}

		float ComputeLocalRadius(const MeshData& data)
		{
			float maxSquared = 0.0f;
			for (const Vertex& vertex : data.Vertices)
			{
				const float squared = glm::dot(vertex.Position, vertex.Position);
				if (squared > maxSquared) maxSquared = squared;
			}
			return std::sqrt(maxSquared);
		}

		float GetMeshRadius(const Mesh* mesh, const MeshData* data)
		{
			if (!mesh) return 1.0f;

			auto it = s.meshRadius.find(mesh);
			if (it != s.meshRadius.end())
				return it->second;

			const float radius = data ? ComputeLocalRadius(*data) : 0.9f;   // fallback cube
			s.meshRadius.emplace(mesh, radius);
			return radius;
		}

		float MaxAxisScale(const Mat4& matrix)
		{
			return std::max(glm::length(Vec3(matrix[0])),
				std::max(glm::length(Vec3(matrix[1])), glm::length(Vec3(matrix[2]))));
		}

		void CollectLights(entt::registry& registry, LightSet& lights)
		{
			if (!s.settings.useEcsLights)
				return;

			auto dirView = registry.view<DirectionalLight, Transform>();
			for (auto [entity, dirLight, transform] : dirView.each())
			{
				if (lights.dirCount >= MaxDirLights) break;

				const Quat rotation = glm::quat(glm::radians(transform.m_Rotation));
				Vec3 direction = rotation * Vec3(0.0f, 0.0f, -1.0f);
				const float length = glm::length(direction);
				direction = length > 0.0001f ? direction / length : Vec3(0.0f, -1.0f, 0.0f);

				lights.dir[lights.dirCount++] = { direction, dirLight.Color, dirLight.Intensity };
			}

			// Point lights: the position comes from the transform of the entity
			auto pointView = registry.view<PointLight, Transform>();
			for (auto [entity, pointLight, transform] : pointView.each())
			{
				if (lights.pointCount >= MaxPointLights) break;

				lights.point[lights.pointCount++] = {
					transform.m_Position,
					pointLight.Color,
					pointLight.Intensity,
					pointLight.Range > 0.0001f ? pointLight.Range : 350.0f
				};
			}
		}

		struct PrepareData
		{
			const EntityItem* items = nullptr;
			u32 count = 0;
			bool culling = false;
			bool useFallbackLight = false;
			Vec3 fallbackDir{ 0.0f, 1.0f, 0.0f };
			Vec3 fallbackColor{ 1.0f };
			const LightSet* lights = nullptr;
			DrawItem* out = nullptr;
			u8* visible = nullptr;
		};

		void PrepareOne(const PrepareData& data, u32 index)
		{
			const EntityItem& item = data.items[index];
			DrawItem& out = data.out[index];

			// ---- 1) culling --------------------------------------------------
			bool visible = true;
			if (data.culling)
				visible = SphereInsideFrustum(item.center, item.radius);

			data.visible[index] = visible ? 1 : 0;
			if (!visible) return;

			// ---- 2) light received by this object ----------------------------
			Vec3 direction(0.0f);
			Vec3 radiance(0.0f);
			bool lit = false;

			if (data.useFallbackLight)
			{
				direction = data.fallbackDir;
				radiance = data.fallbackColor;
				lit = true;
			}
			else
			{
				const LightSet& lights = *data.lights;

				for (u32 i = 0; i < lights.dirCount; i++)
				{
					const DirLightCPU& light = lights.dir[i];
					const Vec3 toLight = -light.direction;      // "towards the light"
					direction += toLight * light.intensity;
					radiance += light.color * light.intensity;
					lit = true;
				}

				for (u32 i = 0; i < lights.pointCount; i++)
				{
					const PointLightCPU& light = lights.point[i];
					const Vec3 toLight = light.position - item.center;
					const float distance = glm::length(toLight);
					if (distance >= light.range) continue;

					// Smooth range cutoff * inverse square falloff (same shape as the
					// legacy shader, so a single point light gives the same image).
					const float scaled = distance / light.range;
					const float window = glm::clamp(1.0f - scaled * scaled * scaled * scaled, 0.0f, 1.0f);
					const float attenuation = (window * window) / std::max(distance * distance, 1.0f);

					const Vec3 contribution = light.color * light.intensity * attenuation;
					radiance += contribution;

					const float weight = std::max(contribution.r, std::max(contribution.g, contribution.b));
					direction += (distance > 0.0001f ? toLight / distance : Vec3(0.0f, 1.0f, 0.0f)) * weight;
					lit = true;
				}

				// A light that reaches the object but has no colour left (e.g. a
				// single point light far away) must not produce a NaN direction.
				if (glm::length(radiance) < 0.00001f)
					lit = false;
			}

			const float dirLength = glm::length(direction);

			// ---- 3) the draw item --------------------------------------------
			out.mesh = item.mesh;
			out.model = item.model;
			out.color = item.color;
			out.params = Vec4(item.roughness, 1.0f, item.texture ? 1.0f : 0.0f, item.metallic);
			out.texture = item.texture;
			out.emissive = item.emissive;
			out.lightDir = Vec4(dirLength > 0.0001f ? direction / dirLength : Vec3(0.0f, 1.0f, 0.0f), lit ? 1.0f : 0.0f);
			out.lightColor = Vec4(radiance, 1.0f);
		}

		void PrepareJob(JobSystem::JobArgs args)
		{
			PrepareData* data = (PrepareData*)args.data;
			PrepareOne(*data, args.jobIndex);
		}

		void PrepareAll(const PrepareData& data)
		{
			for (u32 i = 0; i < data.count; i++)
				PrepareOne(data, i);
		}

		void PrepareAllJobs(const PrepareData& data)
		{
			if (data.count == 0) return;

			const u32 threads = std::max(1u, JobSystem::GetThreadCount());
			const u32 groupSize = glm::clamp(data.count / (threads * 4), 32u, 1024u);

			JobSystem::Counter counter;
			JobSystem::Dispatch(data.count, groupSize, PrepareJob, (void*)&data, &counter);
			JobSystem::WaitForCounter(&counter);
		}

		void GatherEntities(entt::registry& registry)
		{
			s.prepared.clear();

			auto view = registry.view<WorldTransform, MeshRenderer>();

			for (auto entity : view)
			{
				const WorldTransform& transform = view.get<WorldTransform>(entity);
				const MeshRenderer& meshRenderer = view.get<MeshRenderer>(entity);

				const Mesh* mesh = nullptr;
				const MeshData* meshData = nullptr;

				if (auto model = ModelLibrary::Get(meshRenderer.ModelUUID))
				{
					auto& meshes = model->GetMeshes();
					auto& meshesData = model->GetMeshesData();

					if (meshRenderer.MeshIndex < meshes.size())
					{
						mesh = meshes[meshRenderer.MeshIndex].get();

						if (meshRenderer.MeshIndex < meshesData.size())
							meshData = &meshesData[meshRenderer.MeshIndex];
					}
				}

				if (!mesh)
				{
					// Still visible (as a unit cube): an entity of the hierarchy with
					// no usable model must never silently disappear.
					mesh = s.fallbackCube.get();
					if (!mesh) continue;
				}

				auto material = MaterialLibrary::Get(meshRenderer.MaterialUUID);
				if (!material)
					material = MaterialLibrary::Get(UUID(7));   // same fallback as the engine

				EntityItem item;
				item.mesh = mesh;
				item.model = transform.matrix;
				item.center = Vec3(transform.matrix[3]);
				item.radius = GetMeshRadius(mesh, meshData) * MaxAxisScale(transform.matrix);

				if (material)
				{
					const Vec4 color = material->GetColor();
					const float alpha = material->GetAlpha();
					item.color = Vec4(color.r, color.g, color.b, alpha);
					item.roughness = material->GetRough();
					item.metallic = material->GetMetal();
					item.emissive = Vec4(material->GetEmissive(), 1.0f);

					// Albedo map: the unified shader has one sampler (see the note in
					// mesh.frag): the Diffuse channel of the material.
					for (const MapInfo& map : material->GetTextures())
					{
						if (map.type != MapType::Diffuse || !map.useTexture) continue;
						if (auto texture = TextureCache::Get(map.Uuid))
						{
							item.texture = texture.get();
							break;
						}
					}
				}

				s.prepared.push_back(item);
			}
		}
	}

	namespace GfxScene
	{
		void Init()
		{
			if (s.initialized) return;

			s.fallbackCube = CreateCubeMesh();

			// The scene is EMPTY at startup: it contains only what the ECS registry
			// contains (see GatherEntities). Nothing else is added here.
			EQN_CORE_INFO("GfxScene: ready (empty scene - every object comes from the ECS)");

			s.prepStats.threadCount = std::max(1u, JobSystem::GetThreadCount());
			s.prepStats.breakEvenItems = s.settings.jobBreakEvenItems;

			s.initialized = true;
		}

		void Shutdown()
		{
			s.prepared.clear();
			s.results.clear();
			s.visibleFlags.clear();
			s.frustum.clear();
			s.meshRadius.clear();
			s.fallbackCube.reset();
			s.initialized = false;
		}

		void SetTargetSize(u32 width, u32 height)
		{
			if (width == 0 || height == 0) return;
			GfxRenderer::SetSceneTargetSize(width, height);
		}

		void Render(const SceneFrameInput& input, entt::registry& registry)
		{
			if (!s.initialized) return;
			if (!GfxRenderer::IsFrameActive()) return;

			EQN_PROFILE_FUNCTION();

			FrameStats& stats = GfxRenderer::GetStats();

			// ---- 1) the objects of the ECS (meshes, materials, textures) -----
			{
				EQN_PROFILE_SCOPE("GfxScene::Gather");
				GatherEntities(registry);
			}

			// ---- 2) the lights of the hierarchy ------------------------------
			LightSet lights;
			CollectLights(registry, lights);

			const u32 count = (u32)s.prepared.size();

			// ---- 3) preparation of the objects (culling + light) -------------
			const Mat4 viewProj = input.projection * input.view;
			const bool cull = s.settings.culling;
			if (cull)
				ExtractFrustum(viewProj);

			const Vec3 fallbackDir = glm::length(s.settings.lightDirection) > 0.0001f
				? glm::normalize(-s.settings.lightDirection)
				: Vec3(0.0f, 1.0f, 0.0f);

			s.results.resize(count);
			s.visibleFlags.resize(count);

			PrepareData data;
			data.items = s.prepared.data();
			data.count = count;
			data.culling = cull;
			data.useFallbackLight = (lights.Total() == 0);
			data.fallbackDir = fallbackDir;
			data.fallbackColor = s.settings.lightColor;
			data.lights = &lights;
			data.out = s.results.data();
			data.visible = s.visibleFlags.data();

			const u32 threads = std::max(1u, JobSystem::GetThreadCount());

			// Which path is "the chosen one" for this frame (shown in the panel)?
			bool useJobs = false;
			switch (s.settings.jobMode)
			{
			case SceneJobMode::SingleThread: useJobs = false; break;
			case SceneJobMode::Jobs:         useJobs = true;  break;
			default:                         useJobs = (count >= std::max(s.settings.jobBreakEvenItems, SceneSettings::MinItemsForJobs));
				break;
			}

			double singleMs = 0.0;
			double jobsMs = 0.0;

			constexpr u32 kMeasureBothLimit = 2048;
			const bool measuredAB = (count <= kMeasureBothLimit);

			if (measuredAB)
			{
				{
					EQN_PROFILE_SCOPE("GfxScene::Prepare (1 thread)");
					const double start = NowMs();
					PrepareAll(data);
					singleMs = NowMs() - start;
				}
				{
					EQN_PROFILE_SCOPE("GfxScene::Prepare (jobs)");
					const double start = NowMs();
					PrepareAllJobs(data);
					jobsMs = NowMs() - start;
				}

				if (count > 0)
				{
					const u32 minimal = SceneSettings::MinItemsForJobs;
					if (singleMs < jobsMs)
						s.settings.jobBreakEvenItems = std::max(minimal, std::min(s.settings.jobBreakEvenItems * 2, count * 2));
					else
						s.settings.jobBreakEvenItems = std::max(minimal, s.settings.jobBreakEvenItems / 2);
				}
			}
			else
			{
				EQN_PROFILE_SCOPE("GfxScene::Prepare");
				const double start = NowMs();
				if (useJobs) PrepareAllJobs(data);
				else         PrepareAll(data);
				const double elapsed = NowMs() - start;
				if (useJobs) jobsMs = elapsed;
				else         singleMs = elapsed;
			}

			// ---- 4) draw with the current backend ----------------------------
			SceneCamera camera;
			camera.view = input.view;
			camera.projection = input.projection;
			camera.viewProj = viewProj;
			camera.position = input.cameraPosition;

			SceneLighting lighting;
			lighting.direction = fallbackDir;
			lighting.color = s.settings.lightColor;
			lighting.ambient = s.settings.ambient;

			SceneEffects effects = s.settings.effects;
			effects.time = input.time;

			GfxRenderer::BeginScene(camera, lighting, effects, s.settings.clearColor);

			u32 drawn = 0;
			for (u32 i = 0; i < count; i++)
			{
				if (!s.visibleFlags[i]) continue;
				GfxRenderer::DrawItem(s.results[i]);
				drawn++;
			}

			GfxRenderer::EndScene();

			// ---- 5) statistics (the Render panel reads all of this) ----------
			s.singleThreadMs = Smooth(s.singleThreadMs, singleMs);
			s.jobsMs = Smooth(s.jobsMs, jobsMs);

			s.prepStats.singleThreadMs = s.singleThreadMs;
			s.prepStats.jobsMs = s.jobsMs;
			s.prepStats.jobsUsed = useJobs;
			s.prepStats.items = count;
			s.prepStats.visible = drawn;
			s.prepStats.threadCount = threads;
			s.prepStats.breakEvenItems = std::max(s.settings.jobBreakEvenItems, SceneSettings::MinItemsForJobs);
			s.prepStats.measuredAB = measuredAB;

			// Kept for the engine's own counters ("Equinox Metrics")
			stats.scenePrepMs = (float)(useJobs ? s.jobsMs : s.singleThreadMs);
			stats.objectsTotal = count;
			stats.objectsVisible = drawn;

			if (!s.loggedFirstFrame)
			{
				s.loggedFirstFrame = true;
				EQN_CORE_INFO("GfxScene: first frame - {0} object(s) from the ECS, {1} light source(s), {2} worker thread(s), mode {3}",
					count, lights.Total(), threads, useJobs ? "JobSystem" : "1 thread");
			}
		}

		u64 GetSceneTextureID()
		{
			return GfxRenderer::GetSceneTextureId();
		}

		SceneTargetSize GetTargetSize()
		{
			return GfxRenderer::GetSceneTargetSize();
		}

		SceneSettings& GetSettings()
		{
			return s.settings;
		}

		const ScenePrepareStats& GetPrepareStats()
		{
			return s.prepStats;
		}

		u32 GetEntityCount()
		{
			return (u32)s.prepared.size();
		}
	}
}

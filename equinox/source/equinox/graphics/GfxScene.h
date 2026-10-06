#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/core/Math.h"
#include "equinox/graphics/GfxBackend.h" 

#include <entt/entt.hpp>
#include <memory>

namespace Equinox
{
	class Mesh;
	class Texture;
}

namespace Equinox::Gfx
{
	enum class SceneJobMode : int
	{
		SingleThread = 0,
		Jobs,
		Auto
	};

	struct SceneSettings
	{
		Vec3  lightDirection = { -0.4f, -1.0f, -0.3f };
		Vec3  lightColor = { 1.0f, 0.97f, 0.9f };
		float ambient = 0.15f;
		bool  useEcsLights = true;

		SceneJobMode jobMode = SceneJobMode::Auto;
		bool  culling = true;

		Vec4  clearColor = { 0.08f, 0.08f, 0.08f, 1.0f };

		SceneEffects effects;

		u32 jobBreakEvenItems = 256;
		static constexpr u32 MinItemsForJobs = 32;
	};

	struct ScenePrepareStats
	{
		double singleThreadMs = 0.0;   // smoothed time of the "1 thread" path
		double jobsMs = 0.0;           // smoothed time of the JobSystem path
		bool   jobsUsed = false;       // which of the two produced the current frame
		u32    items = 0;              // objects sent by the ECS
		u32    visible = 0;            // objects kept after culling
		u32    threadCount = 1;        // worker threads of the JobSystem
		u32    breakEvenItems = 0;     // current break-even used by "Auto"
		bool   measuredAB = false;     // false when the scene is too big to measure both
	};

	struct SceneFrameInput
	{
		Mat4  view{ 1.0f };
		Mat4  projection{ 1.0f };
		Vec3  cameraPosition{ 0.0f };
		float time = 0.0f;
	};

	namespace GfxScene
	{
		void Init();
		void Shutdown();

		void SetTargetSize(u32 width, u32 height);

		void Render(const SceneFrameInput& input, entt::registry& registry);

		u64 GetSceneTextureID();
		SceneTargetSize GetTargetSize();

		SceneSettings& GetSettings();
		const ScenePrepareStats& GetPrepareStats();

		u32 GetEntityCount();
	}
}
#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/core/Math.h"

#include <memory>
#include <string>
#include <vector>

namespace Equinox
{
	class Mesh;
	class Texture;
}

namespace Equinox::Gfx
{
	enum class BackendType
	{
		OpenGL = 0,
		Vulkan
	};

	struct SceneCamera
	{
		Mat4 view{ 1.0f };
		Mat4 projection{ 1.0f };
		Mat4 viewProj{ 1.0f };
		Vec3 position{ 0.0f };
	};

	struct SceneDirectionalLight
	{
		Vec3 direction{ 0.0f, -1.0f, 0.0f }; // direction in which rays travel
		Vec3 color{ 1.0f };
		float intensity = 1.0f;
	};

	struct ScenePointLight
	{
		Vec3 position{ 0.0f };
		Vec3 color{ 1.0f };
		float intensity = 1.0f;
		float range = 350.0f;
	};

	struct SceneLighting
	{
		static constexpr u32 MaxDirectionalLights = 4;
		static constexpr u32 MaxPointLights = 16;

		Vec3  direction{ -0.4f, -1.0f, -0.3f };
		Vec3  color{ 1.0f, 0.97f, 0.9f };
		float ambient = 0.1f;

		u32 directionalCount = 0;
		u32 pointCount = 0;
		SceneDirectionalLight directional[MaxDirectionalLights];
		ScenePointLight point[MaxPointLights];
	};

	enum class ToneMapOperator : int
	{
		Linear = 0,
		Reinhard,
		ReinhardModified,
		ACES,
		Filmic,
		Uncharted2,
		Count
	};

	struct SceneEffects
	{
		bool  enabled = true;
		float time = 0.0f;

		// ---- tone mapping (memes valeurs que PostProcessPass) ----------------
		float exposure = 1.0f;
		int   toneMapping = (int)ToneMapOperator::ACES;
		float contrast = 2.0f;
		float saturation = 1.0f;

		// ---- colour balance: neutral = (1, 1, 1) ----------------------------
		Vec3  shadowBalance{ 1.0f, 1.0f, 1.0f };
		Vec3  midtoneBalance{ 1.0f, 1.0f, 1.0f };
		Vec3  highlightBalance{ 1.0f, 1.0f, 1.0f };

		// ---- objectif -------------------------------------------------------
		float vignetteAmount = 0.5f;
		float vignetteHardness = 0.25f;
		float grainAmount = 0.03f;
		float aberrationOffset = 0.002f;

		float sharpness = 0.0f;
		float bloomThreshold = 1.0f;
		float bloomStrength = 1.0f;
		int   bloomPasses = 8;
		float ssaoRadius = 0.5f;
		float ssaoIntensity = 0.5f;
		float ssaoBias = 0.025f;

		// 0 = final. Other values are Vulkan debug views selected by RenderPanel.
		int debugView = 0;
	};

	struct SceneTargetSize
	{
		u32 width = 0;
		u32 height = 0;
	};

	struct SceneUniform
	{
		Mat4 viewProj;
		Vec4 cameraPos;
		Vec4 lightDir;
		Vec4 lightColor;
		Vec4 params;

		// Post-process parameters 
		Vec4 post0;       // x = exposure, y = contrast, z = saturation, w = tone mapping operator
		Vec4 post1;       // x = vignette amount, y = vignette hardness, z = grain, w = chromatic aberration
		Vec4 post2;       // rgb = shadows balance, a = 1 when the post-process is enabled
		Vec4 post3;       // rgb = midtones balance
		Vec4 post4;       // rgb = highlights balance
		Vec4 viewport;    // xy = size of the render target (for the screen-space effects)
	};
	static_assert(sizeof(SceneUniform) == 224, "SceneUniform must match the SceneUBO block of mesh.vert/mesh.frag (224 bytes)");

	struct ObjectUniform
	{
		Mat4 model;
		Vec4 color;       // rgb = albedo, a = alpha
		Vec4 params;      // x = roughness, y = uvScale, z = useTexture, w = metallic
		Vec4 lightDir;    // xyz = direction TOWARDS the light, w = 1 if the object is lit
		Vec4 lightColor;  // rgb = radiance accumulated from the ECS lights
		Vec4 emissive;    // rgb = emissive colour of the material
		Vec4 alpha;       // cutoff, has separate alpha map, render mode, alpha-from-diffuse
		Vec4 uvSets;      // diffuse UV index, alpha UV index, reserved, reserved
	};
	static_assert(sizeof(ObjectUniform) == 176, "ObjectUniform must match the ObjectUBO block of vk mesh shaders (176 bytes)");

	// Dynamic offsets (Vulkan and OpenGL) must be aligned; 256 is a valid
	// alignment on every device (maxUniformBufferOffsetAlignment <= 256).
	static constexpr u32 kObjectUniformAlignment = 256;

	struct DrawItem
	{
		const Mesh* mesh = nullptr;
		Mat4  model{ 1.0f };
		Vec4  color{ 1.0f };                        // rgb = albedo (material colour), a = alpha
		Vec4  params{ 0.5f, 1.0f, 1.0f, 0.0f };     // x = roughness, y = uvScale, z = useTexture, w = metallic
		const Texture* texture = nullptr;           // diffuse/albedo texture
		const Texture* alphaTexture = nullptr;      // optional separate opacity map
		u32 diffuseUvIndex = 0;
		u32 alphaUvIndex = 0;

		Vec4  lightDir{ 0.0f, 1.0f, 0.0f, 0.0f };
		Vec4  lightColor{ 1.0f };                   // rgb = incoming radiance
		Vec4  emissive{ 0.0f };                     // rgb = material emission

		// Material alpha/render state required by the Vulkan forward-transparent pass.
		i32 renderMode = 0;                         // RendererAPI::RenderMode numeric value
		float alphaCutoff = 0.5f;
		bool alphaFromDiffuse = false;
		bool transparent = false;
	};

	struct FrameStats
	{
		float cpuFrameMs = 0.0f;     // CPU time of one frame
		float gpuFrameMs = 0.0f;     // GPU time (timestamps) - 0 if unsupported
		float scenePrepMs = 0.0f;    // matrices + culling of the scene
		u32 drawCalls = 0;
		u32 objectsTotal = 0;
		u32 objectsVisible = 0;

		// Detailed counters used by the shared Equinox Metrics panel.
		u32 geometryDrawCalls = 0;
		u32 transparentDrawCalls = 0;
		u32 fullscreenPasses = 0;
		u32 opaqueObjects = 0;
		u32 transparentObjects = 0;
		u32 bloomPasses = 0;
		u32 cachedMeshes = 0;
		u32 cachedTextures = 0;
	};

	struct GPUInfo
	{
		std::string device;
		std::string apiVersion;
		std::string driver;
		float maxAnisotropy = 1.0f;
		bool  timestamps = false;  // GPU timers available
		bool  multiViewport = true;// ImGui windows can be detached (GL only for now)
	};

	class IBackend
	{
	public:
		virtual ~IBackend() = default;

		// ---- identity ----------------------------------------------------
		virtual BackendType GetType() const = 0;
		virtual const char* GetName() const = 0;
		virtual const GPUInfo& GetGPUInfo() const = 0;

		// ---- life cycle --------------------------------------------------
		virtual void Init(void* windowHandle, u32 width, u32 height, bool vsync) = 0;
		virtual void Shutdown() = 0;
		virtual void OnWindowResize(u32 width, u32 height) = 0;
		virtual void SetVSync(bool vsync) = 0;

		// ---- frame -------------------------------------------------------
		// BeginFrame returns false when the frame cannot be drawn (minimized
		// window, swapchain being recreated...): the caller then skips the frame.
		virtual bool BeginFrame() = 0;
		// EndFrame records the UI and PRESENTS (swapchain present for Vulkan,
		// glfwSwapBuffers for OpenGL). Called once per frame, at the very end.
		virtual void EndFrame() = 0;
		virtual bool IsFrameActive() const = 0;

		// ---- scene (offscreen render target shown inside ImGui) ----------
		virtual void SetSceneTargetSize(u32 width, u32 height) = 0;
		virtual SceneTargetSize GetSceneTargetSize() const = 0;

		// Called once per frame, after BeginFrame() and before EndFrame()
		virtual void BeginScene(const SceneCamera& camera, const SceneLighting& lighting,
			const SceneEffects& effects, const Vec4& clearColor) = 0;
		// Fully qualify the type because the virtual method has the same identifier.
		virtual void DrawItem(const Equinox::Gfx::DrawItem& item) = 0;
		virtual void EndScene() = 0;

		// Texture id to give to ImGui::Image() to display the scene (0 = not ready)
		virtual u64 GetSceneTextureId() = 0;

		// ---- ImGui backend ------------------------------------------------
		virtual bool ImGuiInit(void* windowHandle) = 0;
		virtual void ImGuiShutdown() = 0;
		virtual void ImGuiNewFrame() = 0;
		virtual void ImGuiRenderDrawData() = 0;   // Vulkan: the drawing is recorded in EndFrame
		virtual bool IsImGuiReady() const = 0;

		// ---- present surface ("swapchain" in Vulkan) -----------------------
		// Used to configure the ImGui backend. For OpenGL this is the default
		// framebuffer of the window: 1 "image" and no explicit format.
		virtual u32 GetSwapchainImageCount() const = 0;
		virtual u64 GetSwapchainFormat() const = 0;          // API-native format value
		virtual SceneTargetSize GetSwapchainExtent() const = 0;

		// ---- misc ---------------------------------------------------------
		virtual FrameStats& GetStats() = 0;
	};

	namespace Backend
	{
		// Creates AND initializes the backend. Returns false if it failed.
		bool Create(BackendType type, void* windowHandle, u32 width, u32 height, bool vsync);

		bool Exists();
		IBackend& Get();          // asserts if no backend: always call Exists() first
		void Destroy();
	}

	// Small helpers used by the rest of the engine (no API knowledge needed)
	inline const char* ToString(BackendType type) { return type == BackendType::Vulkan ? "Vulkan" : "OpenGL"; }
}

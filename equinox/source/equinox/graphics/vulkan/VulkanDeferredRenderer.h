#pragma once

#include "equinox/graphics/GfxBackend.h"
#include "equinox/graphics/GfxResources.h"
#include "equinox/graphics/GfxPipeline.h"

#include <vulkan/vulkan.h>
#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Equinox::Gfx
{
	// Vulkan-only scene technique. It records this graph in one command buffer:
	// Geometry -> SSAO -> SSAO blur -> deferred lighting -> transparent forward
	// -> bloom extract/blur -> tone mapping/composite.
	class VulkanDeferredRenderer
	{
	public:
		VulkanDeferredRenderer();
		~VulkanDeferredRenderer();

		void Init(u32 width, u32 height);
		void Shutdown();

		void SetTargetSize(u32 width, u32 height);
		SceneTargetSize GetTargetSize() const { return { m_Width, m_Height }; }

		void BeginScene(u32 frameIndex, const SceneCamera& camera, const SceneLighting& lighting,
			const SceneEffects& effects, const Vec4& clearColor);
		// Internal name intentionally differs from the DrawItem type.
		void SubmitItem(const Equinox::Gfx::DrawItem& item);
		void EndScene(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats);

		VkImageView GetFinalView() const { return m_FinalColor.view; }
		VkSampler GetFinalSampler() const { return m_LinearClampSampler; }

	private:
		static constexpr u32 FrameCount = 2;
		static constexpr u32 MaxObjectsPerFrame = 8192;
		static constexpr u32 MaxTextures = 4096;
		static constexpr u32 SsaoKernelSize = 64;

		struct DeferredSceneUniform
		{
			Mat4 view;
			Mat4 projection;
			Mat4 viewProj;
			Vec4 cameraPos;
			Vec4 lightDirAmbient; // xyz = towards light, w = ambient
			Vec4 lightColor;
			Vec4 viewportTime;    // x/y = target size, z = time, w = effects enabled
			Vec4 ssao;            // radius, bias, intensity, kernel size
			Vec4 bloom;           // threshold, strength, pass count, unused
			Vec4 post0;           // exposure, contrast, saturation, tone mapper
			Vec4 post1;           // vignette amount/hardness, grain, aberration
			Vec4 post2;           // shadow balance
			Vec4 post3;           // midtone balance
			Vec4 post4;           // highlight balance, sharpness in w
			Vec4 clearColor;

			// ECS light arrays (same maxima and semantics as the OpenGL LightsUBO).
			Vec4 lightCounts;      // x = directional count, y = point count
			Vec4 dirDirections[SceneLighting::MaxDirectionalLights];
			Vec4 dirColorsIntensity[SceneLighting::MaxDirectionalLights];
			Vec4 pointPositionsRange[SceneLighting::MaxPointLights];
			Vec4 pointColorsIntensity[SceneLighting::MaxPointLights];
		};

		struct SsaoKernelUniform
		{
			Vec4 samples[SsaoKernelSize];
		};

		struct GpuMesh
		{
			GfxBuffer vertices;
			GfxBuffer indices;
			u32 indexCount = 0;
		};

		struct GpuTexture
		{
			GfxImage image;
			VkDescriptorSet set = VK_NULL_HANDLE;
			bool valid = false;
		};

		struct BlurPushConstants
		{
			Vec2 texelSize{ 0.0f };
			i32 horizontal = 0;
			float strength = 1.0f;
		};

		static_assert(sizeof(DeferredSceneUniform) == 1040, "DeferredSceneUniform layout must match Vulkan lighting shaders");
		static_assert(sizeof(BlurPushConstants) == 16, "Bloom push constants must match vk_bloom_blur.frag");

		void CreateTargets(u32 width, u32 height);
		void DestroyTargets();
		void ApplyPendingResize();

		void CreateSamplers();
		void DestroySamplers();
		void CreateDescriptorInfrastructure();
		void DestroyDescriptorInfrastructure();
		void CreateUniformBuffers();
		void DestroyUniformBuffers();
		void AllocateFixedDescriptorSets();
		void UpdateFixedDescriptorSets();
		VkDescriptorSet AllocateSet(VkDescriptorSetLayout layout);
		VkDescriptorSet AllocateTextureSet(VkImageView view);

		void CreatePipelines();
		void DestroyPipelines();

		GpuMesh* GetOrCreateMesh(const Mesh* mesh);
		GpuTexture* GetOrCreateTexture(const Texture* texture);
		GpuTexture* GetWhiteTexture();
		void DestroyResourceCaches();

		void UploadSceneUniform(u32 frameIndex);
		void UploadObjects(u32 frameIndex);
		void BuildDrawLists();
		bool DrawMesh(VkCommandBuffer cmd, u32 frameIndex, u32 itemIndex,
			const GfxPipeline& pipeline, FrameStats& stats);

		void RecordGeometryPass(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats);
		void RecordSsaoPass(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats);
		void RecordSsaoBlurPass(VkCommandBuffer cmd, FrameStats& stats);
		void RecordLightingPass(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats);
		void RecordTransparentPass(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats);
		void RecordBloomPasses(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats);
		void RecordCompositePass(VkCommandBuffer cmd, u32 frameIndex, FrameStats& stats);

		u32 m_Width = 1;
		u32 m_Height = 1;
		u32 m_PendingWidth = 1;
		u32 m_PendingHeight = 1;
		bool m_Initialized = false;
		bool m_SceneActive = false;

		SceneCamera m_Camera;
		SceneLighting m_Lighting;
		SceneEffects m_Effects;
		Vec4 m_ClearColor{ 0.0f };
		std::vector<Equinox::Gfx::DrawItem> m_DrawItems;
		std::vector<u32> m_OpaqueItems;
		std::vector<u32> m_TransparentItems;
		i32 m_LastBloomImage = 0;

		// Render graph resources.
		GfxImage m_GPositionRoughness;
		GfxImage m_GNormalMetallic;
		GfxImage m_GAlbedoAlpha;
		GfxImage m_GEmissive;
		GfxImage m_Depth;
		GfxImage m_SsaoRaw;
		GfxImage m_SsaoBlur;
		GfxImage m_HdrColor;
		GfxImage m_BloomExtract;
		GfxImage m_BloomPing[2];
		GfxImage m_FinalColor;

		VkSampler m_LinearClampSampler = VK_NULL_HANDLE;
		VkSampler m_TextureSampler = VK_NULL_HANDLE;

		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_FrameSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_TextureSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_SsaoSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_SingleTextureSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_LightingSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_BloomExtractSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_CompositeSetLayout = VK_NULL_HANDLE;

		VkDescriptorSet m_FrameSets[FrameCount]{};
		VkDescriptorSet m_SsaoSets[FrameCount]{};
		VkDescriptorSet m_SsaoBlurSet = VK_NULL_HANDLE;
		VkDescriptorSet m_LightingSets[FrameCount]{};
		VkDescriptorSet m_BloomExtractSets[FrameCount]{};
		VkDescriptorSet m_BloomBlurSets[3]{}; // extract, ping 0, ping 1 as source
		VkDescriptorSet m_CompositeSets[FrameCount][2]{};

		GfxBuffer m_SceneUbo[FrameCount];
		GfxBuffer m_ObjectUbo;
		GfxBuffer m_SsaoKernelUbo;

		std::unique_ptr<GfxPipeline> m_GeometryPipeline;
		std::unique_ptr<GfxPipeline> m_SsaoPipeline;
		std::unique_ptr<GfxPipeline> m_SsaoBlurPipeline;
		std::unique_ptr<GfxPipeline> m_LightingPipeline;
		std::unique_ptr<GfxPipeline> m_TransparentPipeline;
		std::unique_ptr<GfxPipeline> m_BloomExtractPipeline;
		std::unique_ptr<GfxPipeline> m_BloomBlurPipeline;
		std::unique_ptr<GfxPipeline> m_CompositePipeline;

		std::unordered_map<const Mesh*, GpuMesh> m_MeshCache;
		std::unordered_map<const Texture*, GpuTexture> m_TextureCache;
		GpuTexture m_WhiteTexture;
	};
}

#pragma once

#include "equinox/ECS/System.h"
#include "equinox/core/Memory.h"
#include "equinox/renderer/rendergraph/RenderGraph.h"
#include "equinox/renderer/backend/vulkan/VulkanPipeline.h"
#include "equinox/renderer/backend/vulkan/VulkanBuffer.h"
#include "equinox/renderer/backend/opengl/GLShader.h"
#include "equinox/renderer/backend/opengl/GLMesh.h"
#include "equinox/renderer/Texture.h"

#include <entt/entt.hpp>
#include <unordered_map>

namespace Equinox
{
    struct GlobalUniforms {
        glm::mat4 viewProjection;
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 cameraPos;
        float time;
    };

    class RenderingSystem : public System
    {
    public:
        RenderingSystem(u32 viewportWidth = 1280, u32 viewportHeight = 720);
        ~RenderingSystem();

        void Update(entt::registry& registry) override;
        void Resize(u32 width, u32 height);

        std::shared_ptr<Texture> GetSceneColor() const { return m_SceneColor; }
        
        u64 GetFrameAllocatorUsage() const { return m_FrameAllocator->GetUsedMemory(); }
        u64 GetFrameAllocatorTotal() const { return m_FrameAllocator->GetTotalSize(); }

    private:
        void InitGlobalUniforms();
        void UpdateGlobalUniforms();

        // [OP] Dual-backend: OpenGL scene framebuffer (SceneColor + depth)
        void CreateGLSceneFramebuffer(u32 width, u32 height);
        void DestroyGLSceneFramebuffer();
        void RecreateGLSceneFramebuffer(u32 width, u32 height);

        // Memory
        std::unique_ptr<LinearAllocator> m_FrameAllocator;

        // Resources
        std::shared_ptr<Texture> m_SceneColor;
        
        std::shared_ptr<VKUniformBuffer> m_GlobalUniformBuffer;
        VkDescriptorSetLayout m_GlobalSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet m_GlobalDescriptorSet = VK_NULL_HANDLE;

        // Vulkan Test Pipeline
        std::unique_ptr<VKPipeline> m_TrianglePipeline;

        // ---- [OP] Dual-backend: OpenGL mirror of the Vulkan PoC ----------
        GLuint m_SceneFBO = 0;
        GLuint m_SceneDepthRBO = 0;
        std::unique_ptr<GLShader> m_TriangleGLShader;
        // Mesh* -> GLMesh (VAO wrapper). Meshes are owned by the Models in
        // the AssetManager, so the pointers stay valid as long as the model
        // is alive. TODO (when Equinox adds asset GC later): invalidate the
        // entries of a model when the model is destroyed.
        std::unordered_map<const Mesh*, std::shared_ptr<GLMesh>> m_GLMeshes;
    };

}

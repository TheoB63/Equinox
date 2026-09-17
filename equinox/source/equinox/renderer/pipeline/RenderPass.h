#pragma once

#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/Framebuffer.h"
#include "equinox/renderer/Shader.h"
#include "equinox/ECS/Components.h"
#include "equinox/core/Math.h"

#include <entt/entt.hpp>
#include <vector>

namespace Equinox
{
	class RenderPipeline;
	
	struct RenderCommand
	{
		entt::entity entity;
		WorldTransform* transform;
		MeshRenderer* meshRend;
		float distance;
	};

	struct RenderContext
	{
        RenderPipeline* pipeline = nullptr;
        entt::registry& registry;
        Vec3 cameraPos;
        std::vector<RenderCommand> opaque;
        std::vector<RenderCommand> transparent;
        u32 width, height;
    };

    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;
        virtual void Init(u32 width, u32 height) = 0;
        virtual void Resize(u32 width, u32 height) = 0;
        virtual void Execute(const RenderContext& ctx) = 0;
        virtual std::vector<std::pair<std::string, u32>> GetAllAttachments() const { return {}; }
    };
}
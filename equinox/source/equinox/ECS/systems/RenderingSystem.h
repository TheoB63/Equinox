#pragma once

#include "equinox/ECS/System.h"

#include "equinox/renderer/RenderTechnique.h"

namespace Equinox
{
	class RenderingSystem : public System
	{
	public:
        RenderingSystem();

        void Update(entt::registry& registry) override;
        void SetTechnique(std::shared_ptr<RenderTechnique> technique);
        std::shared_ptr<RenderTechnique> GetActiveTechnique() { return m_ActiveTechnique; }
        void SetViewProjection(const glm::mat4& vp);
        void Resize(u32 width, u32 height);

    private:
        std::pair<std::vector<RenderCommand>, std::vector<RenderCommand>>
            CollectCommands(entt::registry& registry);

        std::shared_ptr<RenderTechnique> m_ActiveTechnique;
        Vec3 m_CameraPos;
        Mat4 m_ViewProjection;
	};
}
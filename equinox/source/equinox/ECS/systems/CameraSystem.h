#pragma once

#include "equinox/ECS/System.h"
#include "equinox/ECS/Components.h"
#include "equinox/core/Profiler.h"

namespace Equinox
{
    class CameraSystem : public System
    {
    public:
        void Update(Scene* scene) override
        {
            EQN_PROFILE_FUNCTION();

            auto& registry = scene->Registry();
            auto view = registry.view<Camera>();
            for (auto entity : view)
            {
                auto& camera = view.get<Camera>(entity);

                if (camera.IsDirty)
                {
                    if (camera.Projection == Camera::ProjectionType::Perspective)
                    {
                        camera.ProjectionMatrix = glm::perspective(glm::radians(camera.VerticalFOV), camera.AspectRatio, camera.NearClip, camera.FarClip);
                    }
                    else
                    {
                        float orthoLeft = -camera.OrthographicSize * camera.AspectRatio * 0.5f;
                        float orthoRight = camera.OrthographicSize * camera.AspectRatio * 0.5f;
                        float orthoBottom = -camera.OrthographicSize * 0.5f;
                        float orthoTop = camera.OrthographicSize * 0.5f;

                        camera.ProjectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, camera.OrthographicNear, camera.OrthographicFar);
                    }
                    camera.IsDirty = false;
                }
            }
        }
    };
}
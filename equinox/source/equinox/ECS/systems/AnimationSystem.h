#pragma once

#include "equinox/ECS/System.h"

namespace Equinox
{
    class AnimationSystem : public System
    {
    public:
        AnimationSystem() = default;

        void Update(entt::registry& registry) override {}
    };
}

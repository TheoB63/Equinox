#pragma once

#include <entt/entt.hpp>

namespace Equinox
{
    class Scene;

    class System
    {
    public:
        virtual ~System() = default;
        virtual void Update(Scene* scene) = 0;
    };
}

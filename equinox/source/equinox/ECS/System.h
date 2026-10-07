#pragma once

#include <entt/entt.hpp>

namespace Equinox
{
    class System
    {
    public:
        virtual ~System() = default;
        virtual void Update(entt::registry& registry) = 0;
    };
}

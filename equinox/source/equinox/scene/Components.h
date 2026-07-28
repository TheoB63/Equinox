#pragma once

#include "equinox/scene/Entity.h"
#include "equinox/core/UUID.h"

#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace Equinox::Component
{
    struct ID
    {
        UUID m_ID;

        ID() = default;
        ID(const ID&) = default;
    };

    struct Tag
    {
        std::string m_Tag;

        Tag() = default;
        Tag(const Tag&) = default;
        Tag(const std::string& tag) : m_Tag(tag) {}
    };

    struct Parent
    {
        Entity m_Parent;
    };

    struct Children
    {
        std::vector<Entity> m_Children;
    };
}

namespace Equinox { using namespace Equinox::Component; }
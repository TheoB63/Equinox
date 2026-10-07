#include "eqnpch.h"
#include "equinox/ECS/Systems.h"
#include "equinox/ECS/systems/TransformSystem.h"
#include "equinox/ECS/systems/AnimationSystem.h"
#include "equinox/ECS/systems/RenderingSystem.h"

namespace Equinox
{
    std::vector<std::shared_ptr<System>> Systems::s_Systems;
    std::shared_ptr<entt::registry> Systems::s_Registry;

    void Systems::Init() {
        EQN_CORE_INFO("Initializing Systems...");
        AddSystem<TransformSystem>();
        //AddSystem<AnimationSystem>();
        AddSystem<RenderingSystem>();
    }

    void Systems::Shutdown() {
        s_Systems.clear();
        s_Registry.reset();
    }

    void Systems::Update() {
        if (!s_Registry) return;
        for (auto& system : s_Systems) {
            system->Update(*s_Registry);
        }
    }
}

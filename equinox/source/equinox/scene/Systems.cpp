#include "eqnpch.h"
#include "equinox/scene/Systems.h"
#include "equinox/scene/systems/TransformSystem.h"
#include "equinox/scene/systems/AnimationSystem.h"
#include "equinox/scene/systems/CameraSystem.h"
#include "equinox/scene/systems/RenderingSystem.h"

namespace Equinox
{
    std::vector<std::shared_ptr<System>> Systems::s_Systems;
    Scene* Systems::s_Scene = nullptr;

    void Systems::Init() {
        EQN_CORE_INFO("Initializing Systems...");
        AddSystem<TransformSystem>();
        AddSystem<CameraSystem>();
        //AddSystem<AnimationSystem>();
        AddSystem<RenderingSystem>();
    }

    void Systems::Shutdown() {
        s_Systems.clear();
        s_Scene = nullptr;
    }

    void Systems::Update() {
        if (!s_Scene) return;
        for (auto& system : s_Systems) {
            system->Update(s_Scene);
        }
    }
}

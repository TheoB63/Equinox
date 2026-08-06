#include "eqnpch.h"

#include "equinox/scene/Systems.h"

#include "equinox/editor/Editor.h"
#include "equinox/editor/panels/HierarchyPanel.h"

namespace Equinox
{
    // Static member definitions
    std::vector<std::unique_ptr<System>> Systems::s_Systems;
    std::shared_ptr<entt::registry> Systems::s_Registry;

    void Systems::Init()
    {
        s_Registry = Editor::GetPanel<HierarchyPanel>()->GetContext()->RegistryPtr();
        AddSystem<RenderingSystem>();
    }

    void Systems::Shutdown()
    {
        s_Systems.clear();
        s_Registry.reset();
    }

    void Systems::Update()
    {
        for (auto& system : s_Systems)
        {
            system->Update(*s_Registry);
        }
    }
}
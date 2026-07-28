#pragma once

#include "equinox/editor/Editor.h"
#include "equinox/scene/Entity.h"
#include "equinox/scene/Scene.h"

namespace Equinox
{
    class HierarchyPanel : public Panel
    {
    public:
        HierarchyPanel(Scene* context);
        void SetContext(Scene* scene);
        void OnRender() override;

        Entity GetSelectedEntity() const { return m_Selection; }
        void SetSelectedEntity(Entity entity);

    private:
        void DrawEntityNode(Entity entity);
        void DrawEntityContextMenu(Entity entity);
        void DrawEntityCreateMenu();
        void ProcessKeyboardShortcuts();
        bool EntityMatchesFilter(Entity entity);

        Scene* m_Context = nullptr;
        Entity m_Selection;
        char m_SearchFilter[256] = "";
        bool m_ShowCreateMenu = false;
        Entity m_DraggedEntity;
    };
}
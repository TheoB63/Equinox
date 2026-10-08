#pragma once

#include "equinox/editor/Editor.h"
#include "equinox/ECS/Entity.h"
#include "equinox/ECS/Scene.h"

#include <functional>

namespace Equinox
{
    class HierarchyPanel : public Panel
    {
    public:
        HierarchyPanel();

        void OnInit() override;
        void OnRender() override;

        void SetContext(std::shared_ptr<Scene> scene) { m_Context = scene; }

        Entity GetSelectedEntity() const { return m_Selection; }
        void SetSelectedEntity(Entity entity);

    private:
        void DrawTopBar();
        void DrawEntityNode(Entity entity);
        void DrawContextMenu(Entity parent = {});
        
        // Drag & Drop Logic
        void HandleDragDropSource(Entity entity);
        void HandleDragDropTarget(Entity targetEntity);
        void HandleRootDragDropTarget();

        // Helpers
        void RenameEntity(Entity entity);
        void DeleteSelectedEntity();
        bool IsDescendant(Entity potentialDescendant, Entity potentialAncestor);

    private:
        std::shared_ptr<Scene> m_Context;
        Entity m_Selection;
        
        // Renaming
        Entity m_RenamingEntity;
        bool m_IsRenaming = false;
        bool m_FocusRename = false;
        char m_RenameBuffer[256] = "";
        
        char m_SearchFilter[256] = "";
        
        std::vector<std::function<void()>> m_DeferredActions;
    };
}

#pragma once

#include "equinox/core/UUID.h"
#include "equinox/editor/Editor.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace Equinox
{
    class ResourcePanel : public Panel
    {
    public:
        ResourcePanel();

        void OnInit() override;
        void OnRender() override;

    private:
        void DrawFilterControls();
        void SetupColumns();
        void PopulateData();
        void AddModelEntries();
        //void AddTextureEntries();
        //void AddMaterialEntries();
        //void AddShaderEntries();
        ImVec4 GetTypeColor(const std::string& type) const;

        struct ResourceEntry
        {
            std::string Name;
            UUID Uuid;
            std::string Type;
            int RefCount;
        };

        // Filtering state
        char m_SearchBuffer[256] = "";
        bool m_ShowModels = true;
        bool m_ShowTextures = true;
        bool m_ShowMaterials = true;
        bool m_ShowShaders = true;

        // Cached resource data
        std::vector<ResourceEntry> m_FilteredResources;

        // Color theme
        static const std::unordered_map<std::string, ImVec4> m_TypeColors;
    };
}
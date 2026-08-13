#pragma once

#include "equinox/editor/Editor.h"
#include "equinox/resources/FileSystem.h"

namespace Equinox
{
    constexpr const char* ASSET_UUID = "ASSET_UUID";
    constexpr const char* ENTITY_UUID = "ENTITY_UUID";
    
    struct DirectoryNode
    {
        UUID TextureUuid;
        std::string Name;
        ResourceType Type;
        std::vector<DirectoryNode> Directories;
        std::vector<DirectoryNode> Contents;
    };

    class ProjectPanel : public Panel
    {
    public:
        

        ProjectPanel();

        void OnInit() override;
        void OnRender() override;

    private:
        DirectoryNode BuildDirectoryTree(const fs::path& path);
        void DrawDirectoryNode(DirectoryNode& node);
        void DrawPathBar();
        void DrawDirectoryContent();
        void DrawCreateMenu();

        void CreateNewMaterial();

        const DirectoryNode* FindNodeByUuid(const DirectoryNode& node, const UUID& uuid);

    private:
        std::string m_AssetsPath;
        DirectoryNode m_RootNode;
        UUID m_CurrentDirectoryUuid;
    };
}
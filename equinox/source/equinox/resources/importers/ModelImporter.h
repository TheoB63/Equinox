#pragma once

#include "equinox/resources/AssetImporter.h"
#include "equinox/renderer/Model.h" // For MeshData struct

namespace Equinox
{
    struct ModelAssetData : public AssetData
    {
        // We reuse the MeshData struct from Model.h for now
        std::vector<MeshData> Meshes;
        // Add skeleton/animation data here later
    };

    class ModelImporter : public AssetImporter
    {
    public:
        bool Import(const std::filesystem::path& path, std::unique_ptr<AssetData>& outData) override;
    };
}
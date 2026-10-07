#pragma once

#include "equinox/resources/AssetImporter.h"
#include "equinox/renderer/Texture.h"
#include <vector>

namespace Equinox
{
    struct TextureAssetData : public AssetData
    {
        std::vector<unsigned char> Pixels;
        uint32_t Width = 0;
        uint32_t Height = 0;
        TextureFormat Format = TextureFormat::None;
    };

    class TextureImporter : public AssetImporter
    {
    public:
        bool Import(const std::filesystem::path& path, std::unique_ptr<AssetData>& outData) override;
    };
}
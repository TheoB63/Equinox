#pragma once

#include "equinox/resources/Asset.h"
#include <filesystem>
#include <memory>

namespace Equinox
{
    // Intermediate container for raw data loaded from disk (CPU side only)
    struct AssetData
    {
        virtual ~AssetData() = default;
    };

    class AssetImporter
    {
    public:
        virtual ~AssetImporter() = default;

        // Runs on Worker Thread. Reads source, processes, and writes binary artifact to destination.
        // Must NOT touch Vulkan context.
        virtual bool Import(const std::filesystem::path& source, const std::filesystem::path& destination) = 0;
    };
}
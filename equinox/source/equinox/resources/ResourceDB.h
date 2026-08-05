#pragma once

#include "equinox/core/UUID.h"
#include "equinox/resources/MetaFile.h"

#include <filesystem>
#include <unordered_map>

namespace Equinox
{
    class ResourceDB
    {
    public:
        static void Init(const fs::path& projectRoot);

        // UUID <-> Path mapping
        static fs::path ResolveUuid(const UUID& uuid);
        static UUID GetUuidForPath(const fs::path& path);

        // Update operations
        static void RegisterAsset(const fs::path& path, const UUID& uuid);
        static void UnregisterAsset(const fs::path& path);

        // Dependency resolution
        static std::vector<UUID> GetAllDependencies(const UUID& uuid);

    private:
        static bool ProcessMetaFile(const fs::path& path);

    private:
        static std::unordered_map<UUID, fs::path, UUIDHash> s_UuidToPath;
        static std::unordered_map<fs::path, UUID> s_PathToUuid;
    };
}
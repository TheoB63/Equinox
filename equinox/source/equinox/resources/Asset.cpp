#include "eqnpch.h"
#include "equinox/resources/Asset.h"
#include "equinox/resources/AssetDatabase.h"

namespace Equinox
{
    std::string Asset::GetName() const
    {
        if (!Handle.IsValid()) return "Unsaved Asset";
        const auto& meta = AssetDatabase::GetMetadata(Handle);
        if (meta.Path.empty()) return "Unknown Asset";
        return meta.Path.stem().string();
    }
}
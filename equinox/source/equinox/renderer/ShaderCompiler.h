#pragma once

#include "equinox/core/EquinoxTypes.h"
#include <filesystem>
#include <vector>

namespace Equinox
{
    class ShaderCompiler
    {
    public:
        // Returns empty vector on failure
        static std::vector<u32> Compile(const std::filesystem::path& sourcePath, bool optimize = false);
    };
}
#pragma once

#include "equinox/core/UUID.h"

namespace Equinox
{
    class FontViewer
    {
    public:
        void Draw(const UUID& fontUUID, const fs::path& fontPath);
    };
}
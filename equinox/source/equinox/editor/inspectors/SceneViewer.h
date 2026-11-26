#pragma once

#include "equinox/core/UUID.h"

namespace Equinox
{
    class SceneViewer
    {
    public:
        void Draw(const UUID& sceneUUID, const fs::path& scenePath);
    };
}

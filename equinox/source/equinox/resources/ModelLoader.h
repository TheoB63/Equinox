#pragma once

#include "equinox/renderer/Model.h"
#include "equinox/renderer/SkinnedModel.h"

#include <memory>

namespace Equinox
{
    class ModelLoader
    {
    public:
        static std::shared_ptr<Model> Load(const fs::path& path);

    private:
        static bool HasBones(const aiScene* scene);
    };
}
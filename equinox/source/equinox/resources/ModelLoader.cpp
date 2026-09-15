#include "eqnpch.h"
#include "equinox/resources/ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Equinox
{
    std::shared_ptr<Model> ModelLoader::Load(const fs::path& path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.string(),
            aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            EQN_CORE_ERROR("[ModelLoader] Failed to load model: {0}", importer.GetErrorString());
            return nullptr;
        }

        std::shared_ptr<Model> model;

        if (HasBones(scene))
        {
            EQN_CORE_INFO("[ModelLoader] Loading as SkinnedModel");
            auto skinned = std::make_shared<SkinnedModel>(path);
            skinned->SetIsSkinned(true);
            model = skinned;
        }
        else
        {
            EQN_CORE_INFO("[ModelLoader] Loading as static Model");
            model = std::make_shared<Model>(path);
        }

        model->Init();
        return model;
    }

    bool ModelLoader::HasBones(const aiScene* scene)
    {
        for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
        {
            if (scene->mMeshes[i]->HasBones())
                return true;
        }
        return false;
    }
}
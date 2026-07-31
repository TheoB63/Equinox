#include "eqnpch.h"
#include "equinox/renderer/Texture.h"
#include "equinox/renderer/openGL/GLTexture.h"
#include "equinox/renderer/vulkan/VKTexture.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/RendererAPI.h"

namespace Equinox
{
    std::shared_ptr<Texture> Texture::Create(const fs::path& path)
    {
        // Validate path through FileSystem
        const fs::path fullPath = FileSystem::GetPath(Resource::Model, path);

        if (!FileSystem::Validate(fullPath)) {
            EQN_CORE_ERROR("Texture creation failed: Invalid path {}", fullPath);
            return nullptr;
        }

        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::OpenGL:
            return std::make_shared<GLTexture>(fullPath);
        case RendererAPI::API::Vulkan:
            return std::make_shared<VKTexture>(fullPath);
        default:
            EQN_CORE_ASSERT(false, "Unknown renderer API!");
            return nullptr;
        }
    }

    std::shared_ptr<Texture> Texture::Create(uint32_t width, uint32_t height, TextureFormat format)
    {
        if (width == 0 || height == 0) {
            EQN_CORE_ERROR("Texture creation failed: Invalid dimensions {}x{}", width, height);
            return nullptr;
        }

        switch (Renderer::GetAPI())
        {
        case RendererAPI::API::OpenGL:
            return std::make_shared<GLTexture>(width, height, format);
        case RendererAPI::API::Vulkan:
            return std::make_shared<VKTexture>(width, height, format);
        default:
            EQN_CORE_ASSERT(false, "Unknown renderer API!");
            return nullptr;
        }
    }
}
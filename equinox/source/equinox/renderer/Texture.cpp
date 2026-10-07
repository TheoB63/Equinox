#include "eqnpch.h"
#include "equinox/renderer/Texture.h"
#include "equinox/renderer/backend/vulkan/VulkanTexture.h"
#include "equinox/renderer/backend/opengl/GLTexture.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/RendererAPI.h"

namespace Equinox
{
    std::shared_ptr<Texture> Texture::Create(const fs::path& path)
    {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::Vulkan: return std::make_shared<VKTexture>(path);
            // [OP] Dual-backend: OpenGL texture implementation
            case RendererAPI::API::OpenGL: return std::make_shared<GLTexture>(path);
            default:
                EQN_CORE_ASSERT(false, "Unknown renderer API!");
                return nullptr;
        }
    }

    std::shared_ptr<Texture> Texture::Create(u32 width, u32 height, TextureFormat format, const void* data)
    {
        if (width == 0 || height == 0) {
            EQN_CORE_ERROR("Texture creation failed: Invalid dimensions {0}x{1}", width, height);
            return nullptr;
        }

        switch (Renderer::GetAPI()) {
            case RendererAPI::API::Vulkan: return std::make_shared<VKTexture>(width, height, format, data);
            // [OP] Dual-backend: OpenGL texture implementation
            case RendererAPI::API::OpenGL: return std::make_shared<GLTexture>(width, height, format, data);
            default:
                EQN_CORE_ASSERT(false, "Unknown renderer API!");
                return nullptr;
        }
    }
}

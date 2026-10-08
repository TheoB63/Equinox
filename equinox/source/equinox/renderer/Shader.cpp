#include "eqnpch.h"
#include "equinox/renderer/Shader.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/backend/opengl/GLShader.h"
#include "equinox/renderer/backend/vulkan/VulkanShader.h"

namespace Equinox
{
    std::shared_ptr<Shader> Shader::Create(const fs::path& filePath)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::Vulkan:
                return std::make_shared<VulkanShader>(filePath);

            // [OP] Dual-backend: OpenGL shader (GLSL program, see GLShader)
            case RendererAPI::API::OpenGL:
                return std::make_shared<GLShader>(filePath);

            default:
                EQN_CORE_ASSERT(false, "Unknown RendererAPI!");
                return nullptr;
        }
    }

    std::string Shader::Load(const fs::path& filePath)
    {
        std::ifstream in(filePath, std::ios::in | std::ios::binary);
        if (!in) {
            EQN_CORE_ERROR("Could not open shader file: {0}", filePath.string());
            return "";
        }

        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }
}

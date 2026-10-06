#include "eqnpch.h"
#include "equinox/graphics/GfxShader.h"
#include "equinox/core/Log.h"

#include <fstream>

namespace Equinox::Gfx
{
    GfxShader::GfxShader(const std::string& path, ShaderStage stage)
        : m_Stage(stage)
    {
        auto code = ReadFile(path);
        if (!code.empty())
            CreateModule(code);
    }

    GfxShader::~GfxShader()
    {
        if (m_Module)
            vkDestroyShaderModule(GfxContext::Get().GetDevice(), m_Module, nullptr);
    }

    void GfxShader::CreateModule(const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        if (vkCreateShaderModule(GfxContext::Get().GetDevice(), &createInfo, nullptr, &m_Module) != VK_SUCCESS)
        {
            EQN_CORE_CRITICAL("Failed to create shader module!");
        }
    }

    std::vector<char> GfxShader::ReadFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            EQN_CORE_CRITICAL("Failed to open shader file: {0} (did you run CompileShaders.bat ?)", filename);
            return {};
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), (std::streamsize)fileSize);
        return buffer;
    }

    std::string GfxShader::ResolveShaderPath(const std::string& spvFileName)
    {
        fs::path dir = fs::current_path();
        for (int level = 0; level < 5; level++)
        {
            const fs::path candidates[] = {
                dir / "assets" / "shaders" / "spv" / spvFileName,
                dir / "equinox" / "assets" / "shaders" / "spv" / spvFileName,
            };
            for (const auto& c : candidates)
            {
                std::error_code ec;
                if (fs::exists(c, ec)) return c.string();
            }
            if (!dir.has_parent_path() || dir.parent_path() == dir) break;
            dir = dir.parent_path();
        }
        EQN_CORE_ERROR("Shader '{0}' not found (searched assets/shaders/spv from {1} upwards)",
                       spvFileName, fs::current_path().string());
        return {};
    }
}
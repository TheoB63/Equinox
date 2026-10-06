#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/graphics/GfxContext.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace Equinox::Gfx
{
    enum class ShaderStage
    {
        Vertex,
        Fragment,
        Compute
    };

    // A shader = a .spv (SPIR-V) file compiled by CompileShaders.bat from the .vert / .frag
    class GfxShader
    {
    public:
        GfxShader(const std::string& path, ShaderStage stage);
        ~GfxShader();

        GfxShader(const GfxShader&) = delete;
        GfxShader& operator=(const GfxShader&) = delete;

        VkShaderModule GetModule() const { return m_Module; }
        ShaderStage GetStage() const { return m_Stage; }
        const std::string& GetEntryPoint() const { return m_EntryPoint; }

        // Prefers <repository>/equinox/assets/shaders/spv, then checks the
        // sandbox/application asset trees and parent directories. Returns an
        // absolute normalized path, or an empty string with a precise diagnostic.
        static std::string ResolveShaderPath(const std::string& spvFileName);

    private:
        void CreateModule(const std::vector<char>& code);
        std::vector<char> ReadFile(const std::string& filename);

        VkShaderModule m_Module = VK_NULL_HANDLE;
        ShaderStage m_Stage;
        std::string m_EntryPoint = "main";
    };
}
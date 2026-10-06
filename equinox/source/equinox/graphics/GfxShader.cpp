#include "eqnpch.h"
#include "equinox/graphics/GfxShader.h"
#include "equinox/core/Log.h"

#include <fstream>
#include <system_error>

namespace Equinox::Gfx
{
    GfxShader::GfxShader(const std::string& path, ShaderStage stage)
        : m_Stage(stage)
    {
        // ResolveShaderPath deliberately returns an empty string when it cannot
        // find the module. Do not try to open "" and emit a second misleading
        // error after the useful resolver diagnostic.
        if (path.empty()) return;

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
        if ((code.size() % sizeof(u32)) != 0)
        {
            EQN_CORE_ERROR("Invalid SPIR-V byte count: {0}", code.size());
            return;
        }

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const u32*>(code.data());

        if (vkCreateShaderModule(GfxContext::Get().GetDevice(), &createInfo, nullptr, &m_Module) != VK_SUCCESS)
            EQN_CORE_CRITICAL("Failed to create shader module");
    }

    std::vector<char> GfxShader::ReadFile(const std::string& filename)
    {
        if (filename.empty()) return {};

        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            EQN_CORE_ERROR("Failed to open shader file: {0}", filename);
            return {};
        }

        const std::streampos end = file.tellg();
        if (end <= 0)
        {
            EQN_CORE_ERROR("Shader file is empty: {0}", filename);
            return {};
        }

        std::vector<char> buffer((size_t)end);
        file.seekg(0);
        file.read(buffer.data(), (std::streamsize)buffer.size());
        if (!file)
        {
            EQN_CORE_ERROR("Failed to read the complete shader file: {0}", filename);
            return {};
        }
        return buffer;
    }

    std::string GfxShader::ResolveShaderPath(const std::string& spvFileName)
    {
        if (spvFileName.empty()) return {};

        std::error_code ec;
        const fs::path requested(spvFileName);
        if (requested.is_absolute() && fs::is_regular_file(requested, ec))
            return requested.lexically_normal().string();

        const fs::path workingDirectory = fs::current_path(ec);
        fs::path repositoryRoot;

        // Locate the Equinox repository rather than assuming that the process
        // starts in one precise project folder. The sandbox itself also has a
        // premake5.lua, hence the additional "equinox/assets" test.
        fs::path cursor = workingDirectory;
        for (int level = 0; level < 12 && !cursor.empty(); ++level)
        {
            const bool hasWorkspace = fs::exists(cursor / "premake5.lua", ec);
            const bool hasEngineAssets = fs::exists(cursor / "equinox" / "assets" / "shaders", ec);
            if (hasWorkspace && hasEngineAssets)
            {
                repositoryRoot = cursor;
                break;
            }

            const fs::path parent = cursor.parent_path();
            if (parent.empty() || parent == cursor) break;
            cursor = parent;
        }

        auto TryFile = [&](const fs::path& candidate) -> std::string
        {
            ec.clear();
            if (!fs::is_regular_file(candidate, ec)) return {};

            fs::path absolute = fs::absolute(candidate, ec);
            if (ec) absolute = candidate;
            return absolute.lexically_normal().string();
        };

        // Canonical location first. This avoids accidentally loading a stale
        // duplicate from sandbox/assets when both folders contain the shader.
        if (!repositoryRoot.empty())
        {
            const fs::path candidates[] = {
                repositoryRoot / "equinox" / "assets" / "shaders" / "spv" / spvFileName,
                repositoryRoot / "sandbox" / "assets" / "shaders" / "spv" / spvFileName,
                repositoryRoot / "equinoxApp" / "assets" / "shaders" / "spv" / spvFileName,
                repositoryRoot / "assets" / "shaders" / "spv" / spvFileName,
            };
            for (const fs::path& candidate : candidates)
                if (std::string found = TryFile(candidate); !found.empty()) return found;
        }

        // Compatibility with execution from an arbitrary build/bin directory.
        cursor = workingDirectory;
        for (int level = 0; level < 12 && !cursor.empty(); ++level)
        {
            const fs::path candidates[] = {
                cursor / "assets" / "shaders" / "spv" / spvFileName,
                cursor / "equinox" / "assets" / "shaders" / "spv" / spvFileName,
                cursor / "sandbox" / "assets" / "shaders" / "spv" / spvFileName,
            };
            for (const fs::path& candidate : candidates)
                if (std::string found = TryFile(candidate); !found.empty()) return found;

            const fs::path parent = cursor.parent_path();
            if (parent.empty() || parent == cursor) break;
            cursor = parent;
        }

        // Last-resort search, limited to asset trees. This handles a custom
        // project layout without recursively traversing the whole repository.
        if (!repositoryRoot.empty())
        {
            const fs::path assetRoots[] = {
                repositoryRoot / "equinox" / "assets",
                repositoryRoot / "sandbox" / "assets",
                repositoryRoot / "equinoxApp" / "assets",
            };

            for (const fs::path& assetRoot : assetRoots)
            {
                ec.clear();
                if (!fs::is_directory(assetRoot, ec)) continue;

                fs::recursive_directory_iterator iterator(
                    assetRoot, fs::directory_options::skip_permission_denied, ec);
                const fs::recursive_directory_iterator end;
                while (!ec && iterator != end)
                {
                    if (iterator->is_regular_file(ec) && iterator->path().filename() == spvFileName)
                        return fs::absolute(iterator->path(), ec).lexically_normal().string();
                    iterator.increment(ec);
                }
            }
        }

        const fs::path expected = repositoryRoot.empty()
            ? workingDirectory / "assets" / "shaders" / "spv" / spvFileName
            : repositoryRoot / "equinox" / "assets" / "shaders" / "spv" / spvFileName;

        EQN_CORE_ERROR(
            "Shader '{0}' not found. Working directory: '{1}'. Expected canonical path: '{2}'",
            spvFileName, workingDirectory.string(), expected.string());
        return {};
    }
}

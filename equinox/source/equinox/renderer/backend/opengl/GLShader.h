#pragma once

#include "equinox/renderer/Shader.h"

#include <glad/glad.h>
#include <unordered_map>

namespace Equinox
{
    // OpenGL shader (GLSL programs compiled at load time: no SPIRV step).
    //
    // Shader files use the "#type" marker convention:
    //     #type vertex
    //     #version 460 core
    //     ...
    //     #type fragment
    //     #version 460 core
    //     ...
    // (same convention as the Equinox GL shaders). Vulkan shaders stay
    // plain GLSL 450 + SPIR-V (see ShaderCompiler).
    class GLShader : public Shader
    {
    public:
        GLShader(const fs::path& filePath);
        GLShader(const std::string& vertexSrc, const std::string& fragmentSrc);
        ~GLShader() override;

        void Bind() const;
        void Unbind() const;

        void SetBool(const std::string& name, bool value);
        void SetInt(const std::string& name, int value);
        void SetFloat(const std::string& name, float value);
        void SetVec2(const std::string& name, const glm::vec2& vector);
        void SetVec3(const std::string& name, const glm::vec3& vector);
        void SetVec4(const std::string& name, const glm::vec4& vector);
        void SetMat4(const std::string& name, const glm::mat4& matrix);

        const fs::path& GetPath() const override { return m_Path; }

    private:
        std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);
        void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);

        GLenum ShaderTypeFromString(const std::string& type);
        GLint GetUniformLocation(const std::string& name);

        fs::path m_Path;
        GLuint m_ShaderID = 0;
        std::unordered_map<std::string, GLint> m_UniformLocationCache;
    };
}

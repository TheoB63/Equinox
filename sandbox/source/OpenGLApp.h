#pragma once

#include <Equinox.h>

#include <imgui.h>

// TEST
#include <equinox/resources/ShaderLibrary.h>
#include <equinox/resources/ResourceManager.h>

#include <equinox/renderer/Renderer.h>
#include <equinox/renderer/Buffer.h>
#include <equinox/renderer/Shader.h>
#include <equinox/renderer/openGL/GLRendererAPI.h>
#include <equinox/renderer/openGL/GLBuffer.h>
#include <memory>

// TEST OPENGL
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Equinox
{
    struct GLVertex {
        glm::vec2 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
    };

    class OpenGLApp : public App
    {
    public:
        OpenGLApp(int argc, char** argv);
        ~OpenGLApp() override = default;

    protected:
        void OnInit() override;
        void OnUpdate() override;
        void OnUIRender() override;
        void OnShutdown() override;

    private:
        void InitScreenQuad();
        void InitUniformBuffer();
        void UpdateUniforms(const Mat4& model, const Mat4& view, const Mat4& proj);
        void LoadShader();

        GLuint quadVAO, quadVBO;
        GLuint m_UniformBuffer;
        std::shared_ptr<Equinox::Shader> shader;
        fs::path shaderPath;
    };
}
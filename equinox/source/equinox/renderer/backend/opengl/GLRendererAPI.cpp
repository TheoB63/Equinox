#include "eqnpch.h"
#include "equinox/renderer/backend/opengl/GLRendererAPI.h"

#include <glad/glad.h>

#define EQN_GL_CHECK_ERROR() CheckError(__FILE__, __LINE__)

namespace Equinox
{
    void GLRendererAPI::Init(void* windowHandle)
    {
        m_Window = reinterpret_cast<GLFWwindow*>(windowHandle);

        // The window already owns the current GL context (created by
        // WinWindow when the renderer API is OpenGL). Load the GL function
        // pointers with glad and verify it worked.
        if (!gladLoadGL())
        {
            EQN_CORE_CRITICAL("Failed to initialize Glad!");
            return;
        }

        EnableDepthTest(true);
        EnableBlending(true);
        SetBlendFunction(GLBlendFactor::SrcAlpha, GLBlendFactor::OneMinusSrcAlpha);
        SetClearColor({ 0.15f, 0.15f, 0.15f, 1.0f });

        EQN_CORE_INFO("OpenGL Renderer initialized");
        EQN_CORE_TRACE(" - Vendor: {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
        EQN_CORE_TRACE(" - Renderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        EQN_CORE_TRACE(" - Version: {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    }

    void GLRendererAPI::Shutdown()
    {
        if (m_QuadVAO)
        {
            glDeleteVertexArrays(1, &m_QuadVAO);
            glDeleteBuffers(1, &m_QuadVBO);
            m_QuadVAO = 0;
            m_QuadVBO = 0;
        }

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        EQN_CORE_INFO("OpenGL Renderer shut down");
    }

    bool GLRendererAPI::BeginFrame()
    {
        // OpenGL has no fence/semaphore handshake: the frame is always active
        // (a minimized window is skipped by the App, not by the backend).
        return true;
    }

    void GLRendererAPI::EndFrame()
    {
        // Present: for OpenGL the "swapchain" is the default framebuffer.
        glfwSwapBuffers(m_Window);
    }

    void GLRendererAPI::ExecuteGraph(RG::RenderGraph& graph)
    {
        // The RenderGraph passes are recorded with Vulkan commands, so the
        // graph executor is Vulkan-only. The OpenGL backend renders the
        // parallel passes directly from RenderingSystem (see the OpenGL
        // branch of RenderingSystem::Update and README_OGLES.md).
        (void)graph;
        EQN_CORE_ERROR("GLRendererAPI::ExecuteGraph - the RenderGraph executor is Vulkan-only; "
                      "the OpenGL backend renders its parallel passes from RenderingSystem.");
    }

    void GLRendererAPI::OnResize(u32 width, u32 height)
    {
        if (width == 0 || height == 0)
            return;

        // Unlike the Vulkan swapchain there is nothing to recreate: the GL
        // viewport follows the window size.
        SetViewport(0, 0, width, height);
    }

    // ---------------------------------------------------------------------
    // Render state
    // ---------------------------------------------------------------------

    void GLRendererAPI::SetViewport(u32 x, u32 y, u32 width, u32 height)
    {
        glViewport(x, y, width, height);
        EQN_GL_CHECK_ERROR();
    }

    void GLRendererAPI::SetClearColor(const Vec4& color)
    {
        m_ClearColor = color;
        glClearColor(color.r, color.g, color.b, color.a);
        EQN_GL_CHECK_ERROR();
    }

    void GLRendererAPI::Clear(GLBufferBit bits)
    {
        using Underlying = std::underlying_type_t<GLBufferBit>;
        Underlying bitsValue = static_cast<Underlying>(bits);

        GLbitfield clearMask = 0;

        if (bitsValue & static_cast<Underlying>(GLBufferBit::Color))
            clearMask |= GL_COLOR_BUFFER_BIT;
        if (bitsValue & static_cast<Underlying>(GLBufferBit::Depth))
            clearMask |= GL_DEPTH_BUFFER_BIT;
        if (bitsValue & static_cast<Underlying>(GLBufferBit::Stencil))
            clearMask |= GL_STENCIL_BUFFER_BIT;

        glClear(clearMask);
        EQN_GL_CHECK_ERROR();
    }

    void GLRendererAPI::EnableDepthMask(bool enable)
    {
        glDepthMask(enable ? GL_TRUE : GL_FALSE);
        EQN_GL_CHECK_ERROR();
    }

    bool GLRendererAPI::IsDepthMaskEnabled()
    {
        GLboolean currentState;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &currentState);
        EQN_GL_CHECK_ERROR();
        return currentState == GL_TRUE;
    }

    void GLRendererAPI::EnableDepthTest(bool enable)
    {
        m_DepthTestEnabled = enable;
        enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
        EQN_GL_CHECK_ERROR();
    }

    void GLRendererAPI::EnableBlending(bool enable)
    {
        m_BlendingEnabled = enable;
        enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        EQN_GL_CHECK_ERROR();
    }

    void GLRendererAPI::SetBlendFunction(GLBlendFactor srcFactor, GLBlendFactor dstFactor)
    {
        GLenum glSrc = BlendFactorToGL(srcFactor);
        GLenum glDst = BlendFactorToGL(dstFactor);
        glBlendFunc(glSrc, glDst);
        EQN_GL_CHECK_ERROR();
    }

    // ---------------------------------------------------------------------
    // Fullscreen quad (screen-space passes)
    // ---------------------------------------------------------------------

    void GLRendererAPI::InitFullscreenQuad()
    {
        if (m_QuadVAO == 0)
        {
            float quadVertices[] =
            {
                // Positions   // TexCoords
                -1.0f,  1.0f,  0.0f, 1.0f,
                -1.0f, -1.0f,  0.0f, 0.0f,
                 1.0f, -1.0f,  1.0f, 0.0f,

                -1.0f,  1.0f,  0.0f, 1.0f,
                 1.0f, -1.0f,  1.0f, 0.0f,
                 1.0f,  1.0f,  1.0f, 1.0f
            };

            glGenVertexArrays(1, &m_QuadVAO);
            glGenBuffers(1, &m_QuadVBO);
            glBindVertexArray(m_QuadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

            // Positions
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            // Texture Coords
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

            glBindVertexArray(0);
        }
        EQN_GL_CHECK_ERROR();
    }

    void GLRendererAPI::DrawFullscreenQuad()
    {
        if (m_QuadVAO == 0) InitFullscreenQuad();
        glBindVertexArray(m_QuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        EQN_GL_CHECK_ERROR();
    }

    // ---------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------

    GLenum GLRendererAPI::BlendFactorToGL(GLBlendFactor factor) const
    {
        switch (factor)
        {
        case GLBlendFactor::Zero:               return GL_ZERO;
        case GLBlendFactor::One:                return GL_ONE;
        case GLBlendFactor::SrcAlpha:           return GL_SRC_ALPHA;
        case GLBlendFactor::OneMinusSrcAlpha:   return GL_ONE_MINUS_SRC_ALPHA;
        case GLBlendFactor::DstAlpha:           return GL_DST_ALPHA;
        case GLBlendFactor::OneMinusDstAlpha:   return GL_ONE_MINUS_DST_ALPHA;
        default:
            EQN_CORE_ERROR("Unsupported blend factor: {0}", static_cast<int>(factor));
            return GL_ONE;
        }
    }

    void GLRendererAPI::CheckError(const char* file, int line)
    {
        while (GLenum error = glGetError())
        {
            std::string errorStr;
            switch (error)
            {
            case GL_INVALID_ENUM:      errorStr = "INVALID_ENUM";      break;
            case GL_INVALID_VALUE:     errorStr = "INVALID_VALUE";     break;
            case GL_INVALID_OPERATION: errorStr = "INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY:     errorStr = "OUT_OF_MEMORY";     break;
            default:                   errorStr = "UNKNOWN_ERROR";     break;
            }
            EQN_CORE_ERROR("OpenGL Error ({0}) at {1}:{2}", errorStr, file, line);
        }
    }
}

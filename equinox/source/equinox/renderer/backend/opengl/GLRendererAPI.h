#pragma once

#include "equinox/renderer/RendererAPI.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Equinox
{
    // OpenGL equivalents of the render-target bits (Vulkan expresses these
    // natively through the render pass attachments).
    enum class GLBufferBit
    {
        None = 0,
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2
    };

    inline GLBufferBit operator|(GLBufferBit lhs, GLBufferBit rhs)
    {
        return static_cast<GLBufferBit>(
            static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs));
    }

    enum class GLBlendFactor
    {
        Zero,
        One,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha
    };

    // OpenGL backend of the RendererAPI.
    //
    // NOTE (dual-backend): at this stage of Equinox the RenderGraph passes are
    // recorded with Vulkan commands, so the graph itself is executed by the
    // Vulkan backend. The OpenGL backend renders the SAME passes directly
    // from RenderingSystem (GL mirror), keeping both APIs in sync step by
    // step. See README_OGLES.md.
    class GLRendererAPI : public RendererAPI
    {
    public:
        GLRendererAPI() = default;

        virtual void Init(void* windowHandle) override;
        virtual void Shutdown() override;

        virtual bool BeginFrame() override;
        virtual void EndFrame() override;
        virtual void ExecuteGraph(RG::RenderGraph& graph) override;
        virtual void OnResize(u32 width, u32 height) override;

        // ---- Render state (OpenGL side of the dual-backend API) ----------
        void SetViewport(u32 x, u32 y, u32 width, u32 height);
        void SetClearColor(const Vec4& color);
        void Clear(GLBufferBit bits);

        void EnableDepthMask(bool enable);
        bool IsDepthMaskEnabled();
        void EnableDepthTest(bool enable);
        bool IsDepthTestEnabled() const { return m_DepthTestEnabled; }

        void EnableBlending(bool enable);
        void SetBlendFunction(GLBlendFactor srcFactor, GLBlendFactor dstFactor);

        // Fullscreen quad (screen-space passes: post process, blit, ...)
        void InitFullscreenQuad();
        void DrawFullscreenQuad();

        GLFWwindow* GetWindow() const { return m_Window; }

    private:
        GLenum BlendFactorToGL(GLBlendFactor factor) const;
        void CheckError(const char* file, int line);

        GLFWwindow* m_Window = nullptr;

        bool m_DepthTestEnabled = true;
        bool m_BlendingEnabled = true;
        Vec4 m_ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

        GLuint m_QuadVAO = 0;
        GLuint m_QuadVBO = 0;
    };
}

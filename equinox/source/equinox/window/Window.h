#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/renderer/RendererAPI.h"

#include <functional>
#include <memory>

namespace Equinox
{
    struct WindowSpec
    {
        std::string Title = "Equinox Engine";
        u32 Width = 1280;
        u32 Height = 720;
        bool VSync = false;
        bool Fullscreen = false;

        // [OP] Dual-backend: which graphics API this window is created for.
        // OpenGL windows get a GL context (GLFW default client API + 4.6
        // core profile); Vulkan windows stay context-less (GLFW_NO_API).
        // Default: Vulkan (Equinox's main API).
        RendererAPI::API rendererAPI = RendererAPI::API::Vulkan;
    };

    class Window
    {
    public:
        virtual ~Window() = default;

        virtual void OnUpdate() = 0;
        virtual void SwapBuffers() = 0;

        virtual void SetVSync(bool enabled) = 0;;
        virtual void ToggleFullscreen() = 0;

        virtual u32 GetWidth() const = 0;
        virtual u32 GetHeight() const = 0;
        virtual void* GetNativeWindow() const = 0;

		virtual void SetWindowColors(const Vec3& caption, const Vec3& border, const Vec3& text) = 0;

        virtual bool IsMinimized() = 0;

        static std::unique_ptr<Window> Create(const WindowSpec& spec = WindowSpec());
    };
}

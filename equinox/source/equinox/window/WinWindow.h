#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/window/Window.h"

#include <GLFW/glfw3.h>

namespace Equinox
{
    class WinWindow : public Window
    {
    public:
        WinWindow(const WindowSpec& spec);
        virtual ~WinWindow();

        void OnUpdate() override;
        void SwapBuffers();

        void SetVSync(bool enabled) override;
        void ToggleFullscreen() override;

        bool IsMinimized() override;

        u32 GetWidth() const override { return m_Data.Width; }
        u32 GetHeight() const override { return m_Data.Height; }
        void* GetNativeWindow() const override { return m_GLFWwindow; }

    private:
        void Init(const WindowSpec& spec);
        void Shutdown();

        GLFWwindow* m_GLFWwindow = nullptr;

        struct WindowData
        {
            std::string Title;
            u32 Width;
            u32 Height;
            bool VSync;
            bool Fullscreen;
            std::shared_ptr<EventBus> EventBus;
        };

        WindowData m_Data;
    };
}
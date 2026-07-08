#include "eqnpch.h"
#include "equinox/input/Input.h"
#include "equinox/ui/UI.h"

#include <GLFW/glfw3.h>

namespace Equinox
{
    void Input::SetWindow(void* window)
    {
        s_Window = window;
    }

    bool Input::IsKeyPressed(int keycode)
    {
        if (UI::WantCaptureKeyboard()) return false;
        return glfwGetKey(static_cast<GLFWwindow*>(s_Window), keycode) == GLFW_PRESS;
    }

    bool Input::IsMouseButtonPressed(int button)
    {
        if (UI::WantCaptureMouse()) return false;
        return glfwGetMouseButton(static_cast<GLFWwindow*>(s_Window), button) == GLFW_PRESS;
    }

    Vec2 Input::GetMousePosition()
    {
        double x, y;
        glfwGetCursorPos(static_cast<GLFWwindow*>(s_Window), &x, &y);
        return { static_cast<f32>(x), static_cast<f32>(y) };
    }
}
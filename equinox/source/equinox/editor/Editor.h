#pragma once

#include "equinox/core/EquinoxTypes.h"

struct ImGuiContext;

namespace Equinox
{
    class Editor
    {
    public:
        static void Init(void* window);
        static void Shutdown();

        static void BeginFrame();
        static void EndFrame();

        static bool WantCaptureMouse();
        static bool WantCaptureKeyboard();

    private:
        static inline ImGuiContext* s_Context = nullptr;
    };
}
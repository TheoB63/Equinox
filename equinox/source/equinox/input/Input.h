#pragma once

#include "equinox/core/EquinoxTypes.h"

namespace Equinox
{
    class Input
    {
    public:
        static void SetWindow(void* window);

        static bool IsKeyPressed(int keycode);
        static bool IsMouseButtonPressed(int button);

        static Vec2 GetMousePosition();

    private:
        static inline void* s_Window = nullptr;
    };
}

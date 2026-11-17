#include "eqnpch.h"
#include "equinox/platform/Window.h"
#include "equinox/platform/WinWindow.h"

namespace Equinox
{
    std::unique_ptr<Window> Window::Create(const WindowSpec& spec)
    {
        return std::make_unique<WinWindow>(spec);
    }
}

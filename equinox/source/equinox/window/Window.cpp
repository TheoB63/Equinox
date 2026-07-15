#include "eqnpch.h"
#include "equinox/window/Window.h"
#include "equinox/window/WinWindow.h"

namespace Equinox
{
	std::unique_ptr<Window> Window::Create(const WindowSpec& spec)
	{
		return std::make_unique<WinWindow>(spec);
	}
}
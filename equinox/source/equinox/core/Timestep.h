#pragma once

#include "equinox/core/EquinoxTypes.h"

#include "GLFW/glfw3.h"

namespace Equinox
{
	class Timestep
	{
	public:
		static f32 GetTime() { return glfwGetTime(); }
		static f32 GetTimeMS() { return glfwGetTime() * 1000.0f; }
	};
}
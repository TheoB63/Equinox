#include <Equinox.h>
#include "VulkanApp.h"

#include <imgui.h>

// TEST
#include <equinox/resources/ShaderLibrary.h>
#include <equinox/resources/ResourceManager.h>

#include <equinox/renderer/Renderer.h>
#include <equinox/renderer/vulkan/VKRendererAPI.h>
#include <equinox/renderer/Shader.h>
#include <memory>

// TEST VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Equinox
{
	VulkanApp::VulkanApp(int argc, char** argv) : App(argc, argv) {}

	void VulkanApp::OnInit()
	{
	}

	void VulkanApp::OnUpdate(f32 dt)
	{
		static float time = 0;
		time += dt;
	}

	void VulkanApp::OnUIRender()
	{
		// ImGui Demo
		static bool showDemo = true;
		if (showDemo) ImGui::ShowDemoWindow(&showDemo);
	}

	void VulkanApp::OnShutdown()
	{
		//vkDestroyInstance(instance, nullptr);
	}
}
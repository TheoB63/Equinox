#pragma once

#include <Equinox.h>

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
	class VulkanApp : public App
	{
	public:
		VulkanApp(int argc, char** argv);
		~VulkanApp() override = default;
	
	protected:
		void OnInit() override;
		void OnUpdate(f32 dt) override;
		void OnUIRender() override;
		void OnShutdown() override;
	};
}
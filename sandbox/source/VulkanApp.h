#pragma once

#include <Equinox.h>

#include <imgui.h>

// TEST 
#include <equinox/resources/ShaderLibrary.h>
#include <equinox/resources/ResourceManager.h>

#include <equinox/renderer/Renderer.h>
#include <equinox/renderer/Buffer.h>
#include <equinox/renderer/Shader.h>
#include <equinox/renderer/vulkan/VKRendererAPI.h>
#include <equinox/renderer/vulkan/VKBuffer.h>
#include <memory>

// TEST VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Equinox
{
	struct VKVertex {
		glm::vec2 pos;
		glm::vec3 color;
	};

	class VulkanApp : public App
	{
	public:
		VulkanApp(int argc, char** argv);
		~VulkanApp() override = default;
	
	protected:
		void OnInit() override;
		void OnUpdate() override;
		void OnUIRender() override;
		void OnShutdown() override;
	};
}
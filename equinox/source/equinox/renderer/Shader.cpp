#include "eqnpch.h"
#include "equinox/renderer/Shader.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/RenderBackend.h"
#include "equinox/renderer/backend/vulkan/VulkanShader.h"

namespace Equinox
{
	std::shared_ptr<Shader> Shader::Create(const fs::path& filePath)
	{
		switch (Renderer::GetBackend()->GetAPI())
		{
		case RenderBackend::API::Vulkan:
			return std::make_shared<VulkanShader>(filePath);

		default:
			EQN_CORE_ASSERT(false, "Unknown RenderBackend!");
			return nullptr;
		}
	}

	std::shared_ptr<Shader> Shader::Create(const std::vector<u32>& vertSpv, const std::vector<u32>& fragSpv, const fs::path& path)
	{
		switch (Renderer::GetBackend()->GetAPI())
		{
		case RenderBackend::API::Vulkan:
			return std::make_shared<VulkanShader>(vertSpv, fragSpv, path);
		default:
			EQN_CORE_ASSERT(false, "Unknown RenderBackend!");
			return nullptr;
		}
	}
}

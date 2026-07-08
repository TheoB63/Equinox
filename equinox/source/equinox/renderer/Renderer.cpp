#include "eqnpch.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/openGL/GLRenderer.h"

namespace Equinox
{
	RendererAPI Renderer::s_API = RendererAPI::OpenGL;

	RendererAPI Renderer::GetAPI()
	{
		return s_API;
	}

	std::unique_ptr<Renderer> Renderer::Create()
	{
		switch (s_API)
		{
		case RendererAPI::OpenGL:
			return std::make_unique<GLRenderer>();
		case RendererAPI::Vulkan:
			LH_CORE_ASSERT(false,"RendererAPI::Vulkan is not supported!");
			return nullptr;
		case RendererAPI::None:
			LH_CORE_ASSERT(false, "RendererAPI::None is not supported!");
			return nullptr;
		default:
			LH_CORE_ASSERT(false, "Unknown RendererAPI!");
			return nullptr;
		}
	}
}
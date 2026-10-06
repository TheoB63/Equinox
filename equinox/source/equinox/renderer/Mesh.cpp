#include "eqnpch.h"

#include "equinox/renderer/Mesh.h"
#include "equinox/renderer/openGL/GLMesh.h"
#include "equinox/renderer/null/NullResources.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/RendererAPI.h"

namespace Equinox
{
	std::shared_ptr<Mesh> Mesh::Create(
		const std::shared_ptr<VertexBuffer>& vb,
		const std::shared_ptr<IndexBuffer>& ib)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
		{
			auto glVB = std::dynamic_pointer_cast<GLVertexBuffer>(vb);
			auto glIB = std::dynamic_pointer_cast<GLIndexBuffer>(ib);

			EQN_CORE_ASSERT(glVB, "VertexBuffer is not a GLVertexBuffer!");
			EQN_CORE_ASSERT(glIB, "IndexBuffer is not a GLIndexBuffer!");

			return std::make_shared<GLMesh>(glVB, glIB);
		}

		case RendererAPI::API::Vulkan:
			return std::make_shared<NullMesh>(vb, ib);

		default:
			EQN_CORE_ASSERT(false, "Unknown renderer API!");
			return nullptr;
		}
	}
}
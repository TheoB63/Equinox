#include "eqnpch.h"

#include "equinox/renderer/Mesh.h"
#include "equinox/renderer/Renderer.h"
#include "equinox/renderer/RendererAPI.h"

namespace Equinox
{
    std::shared_ptr<Mesh> Mesh::Create(
        const std::shared_ptr<VertexBuffer>& vb,
        const std::shared_ptr<IndexBuffer>& ib)
    {
        return std::make_shared<Mesh>(vb, ib);
    }
}

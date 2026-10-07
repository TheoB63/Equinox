#pragma once

// "Null" GPU resources: empty implementations returned by the factories when
// the active backend does not use this resource type (Vulkan has no VAO, so
// VertexArray::Create() returns a NullVertexArray).
//
// This file is the trimmed Equinox-side equivalent of the Equinox
// renderer/null/NullResources.h: only what Equinox's base interfaces need at
// this stage is kept.

#include "equinox/renderer/VertexArray.h"

#include <vector>
#include <memory>

namespace Equinox
{
    // ---------------------------------------------------------------- VertexArray
    class NullVertexArray : public VertexArray
    {
    public:
        void Bind() const override {}
        void Unbind() const override {}

        void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vb) override { m_VertexBuffers.push_back(vb); }
        void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& ib) override { m_IndexBuffer = ib; }

        const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const override { return m_VertexBuffers; }
        const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

    private:
        std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
        std::shared_ptr<IndexBuffer> m_IndexBuffer;
    };
}

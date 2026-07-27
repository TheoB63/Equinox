#pragma once

#include "equinox/renderer/Buffer.h"
#include "equinox/renderer/Texture.h"

#include <memory>

namespace Equinox
{
    class Mesh
    {
    public:
        virtual ~Mesh() = default;
        virtual void Bind() const = 0;
        virtual void Draw() const = 0;

        static std::shared_ptr<Mesh> Create(
            const std::shared_ptr<VertexBuffer>& vb,
            const std::shared_ptr<IndexBuffer>& ib = nullptr,
            const std::shared_ptr<Texture>& texture = nullptr
        );
    };
}
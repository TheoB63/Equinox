#pragma once

#include "equinox/renderer/VertexBuffer.h"

namespace Equinox
{
    class GLVertexBuffer : public VertexBuffer
    {
    public:
        GLVertexBuffer(uint32_t size);
        GLVertexBuffer(const void* data, uint32_t size);
        ~GLVertexBuffer();

        void Bind() const override;
        void Unbind() const override;
        void SetData(const void* data, uint32_t size) override;

    private:
        uint32_t m_RendererID;
    };
}
#pragma once

#include "equinox/renderer/Mesh.h"

#include <glad/glad.h>

namespace Equinox
{
    // OpenGL mesh: wraps the (GL) vertex/index buffers in a VAO so a draw is
    // a single glBindVertexArray + glDrawElements.
    class GLMesh : public Mesh
    {
    public:
        GLMesh(const std::shared_ptr<VertexBuffer>& vertexBuffer,
               const std::shared_ptr<IndexBuffer>& indexBuffer)
            : Mesh(vertexBuffer, indexBuffer)
        {
            CreateVAO();
        }

        // NOTE: Mesh::~Mesh is not virtual, so a GLMesh must be destroyed
        // through a shared_ptr<GLMesh> (the RenderingSystem mesh cache keeps
        // the static type).
        ~GLMesh()
        {
            if (m_VAO)
                glDeleteVertexArrays(1, &m_VAO);
        }

        void Bind() const;
        void Draw() const;

    private:
        void CreateVAO();
        static GLenum ShaderDataTypeToGLType(ShaderDataType type);

        GLuint m_VAO = 0;
    };
}

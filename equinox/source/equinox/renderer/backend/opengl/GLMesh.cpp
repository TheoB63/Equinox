#include "eqnpch.h"
#include "equinox/renderer/backend/opengl/GLMesh.h"

namespace Equinox
{
    void GLMesh::Bind() const
    {
        glBindVertexArray(m_VAO);
    }

    void GLMesh::Draw() const
    {
        glBindVertexArray(m_VAO);
        if (GetIndexBuffer())
        {
            glDrawElements(GL_TRIANGLES, GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
        }
        // No index buffer: the vertex count is not tracked by the base
        // VertexBuffer; indexed models are the common case (assimp exports
        // always index their meshes).
    }

    void GLMesh::CreateVAO()
    {
        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        auto vertexBuffer = GetVertexBuffer();
        vertexBuffer->Bind();

        const auto& layout = vertexBuffer->GetLayout();
        uint32_t index = 0;
        uint32_t stride = layout.GetStride();

        for (const auto& element : layout.GetElements())
        {
            glEnableVertexAttribArray(index);
            GLenum glType = ShaderDataTypeToGLType(element.Type);
            if (glType == GL_INT || glType == GL_UNSIGNED_INT)
            {
                glVertexAttribIPointer(
                    index,
                    element.GetComponentCount(),
                    glType,
                    stride,
                    (const void*)(intptr_t)element.Offset);
            }
            else
            {
                glVertexAttribPointer(
                    index,
                    element.GetComponentCount(),
                    glType,
                    element.Normalized ? GL_TRUE : GL_FALSE,
                    stride,
                    (const void*)(intptr_t)element.Offset);
            }
            index++;
        }

        if (GetIndexBuffer())
            GetIndexBuffer()->Bind();

        glBindVertexArray(0);
    }

    GLenum GLMesh::ShaderDataTypeToGLType(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Float:    return GL_FLOAT;
        case ShaderDataType::Float2:   return GL_FLOAT;
        case ShaderDataType::Float3:   return GL_FLOAT;
        case ShaderDataType::Float4:   return GL_FLOAT;
        case ShaderDataType::Int:      return GL_INT;
        case ShaderDataType::Int2:     return GL_INT;
        case ShaderDataType::Int3:     return GL_INT;
        case ShaderDataType::Int4:     return GL_INT;
        case ShaderDataType::Mat3:     return GL_FLOAT;
        case ShaderDataType::Mat4:     return GL_FLOAT;
        case ShaderDataType::Bool:     return GL_BOOL;
        default:
            EQN_CORE_ASSERT(false, "Unknown ShaderDataType!");
            return 0;
        }
    }
}

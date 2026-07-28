#pragma once

#include "equinox/core/Log.h"
#include "equinox/renderer/Mesh.h"
#include "equinox/renderer/openGL/GLBuffer.h"
#include "equinox/renderer/openGL/GLTexture.h"

#include <glad/glad.h>
#include <vector>
#include <memory>

namespace Equinox
{
	class GLMesh : public Mesh
	{
	public:
		GLMesh(const std::shared_ptr<GLVertexBuffer>& vertexBuffer,
			const std::shared_ptr<GLIndexBuffer>& indexBuffer,
			const std::shared_ptr<Material> material);

		void Bind() const override;
		void Draw() const override;

	private:
		void CreateVAO();
		static GLenum ShaderDataTypeToGLType(ShaderDataType type);

		GLuint m_VAO;
		std::shared_ptr<GLVertexBuffer> m_VertexBuffer;
		std::shared_ptr<GLIndexBuffer> m_IndexBuffer;
		std::shared_ptr<Material> m_Material;
	};
}
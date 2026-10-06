#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/renderer/Texture.h"
#include "equinox/renderer/Shader.h"
#include "equinox/renderer/Buffer.h"
#include "equinox/renderer/VertexArray.h"
#include "equinox/renderer/Mesh.h"
#include "equinox/resources/FileSystem.h"
#include "equinox/utils/ImageUtils.h"

#include <vector>
#include <memory>
#include <cstring>

namespace Equinox
{
	class NullTexture : public Texture
	{
	public:
		explicit NullTexture(const fs::path& path)
			: m_Path(FileSystem::GetPath(ResourceType::Texture, path))
		{
			// Read only the image header (no pixel decoding) to know the size and channel count
			int w = 0, h = 0, channels = 0;
			if (stbi_info(m_Path.string().c_str(), &w, &h, &channels))
			{
				m_Width = (u32)w;
				m_Height = (u32)h;
				m_Format = channels == 1 ? TextureFormat::R8 : (channels == 3 ? TextureFormat::RGB8 : TextureFormat::RGBA8);
			}
		}

		NullTexture(u32 width, u32 height, TextureFormat format)
			: m_Width(width), m_Height(height), m_Format(format) {}

		// Procedural texture (created from pixels, not from a file): the pixels are
		// kept on the CPU side so that the Vulkan renderer can upload them later.
		NullTexture(u32 width, u32 height, TextureFormat format, const void* data)
			: m_Width(width), m_Height(height), m_Format(format)
		{
			if (data && format == TextureFormat::RGBA8)
			{
				const u8* bytes = (const u8*)data;
				m_Pixels.assign(bytes, bytes + (size_t)width * (size_t)height * 4);
			}
		}

		void Bind(u32 slot = 0) const override {}

		u32 GetWidth() const override { return m_Width; }
		u32 GetHeight() const override { return m_Height; }
		u32 GetRendererID() const override { return 0; }   // 0 = "no GPU texture" (GfxImGui draws a placeholder)
		const fs::path& GetPath() const override { return m_Path; }

		TextureFormat GetFormat() const override { return m_Format; }
		std::string GetFormatString() const override
		{
			switch (m_Format)
			{
			case TextureFormat::R8:      return "R8";
			case TextureFormat::RGB8:    return "RGB8";
			case TextureFormat::RGBA8:   return "RGBA8";
			case TextureFormat::RGBA32F: return "RGBA32F";
			default: return "Unknown";
			}
		}

		TextureWrapMode GetWrapMode() const override { return m_WrapMode; }
		void SetWrapMode(TextureWrapMode mode) override { m_WrapMode = mode; }

		std::pair<TextureFilterMode, TextureFilterMode> GetFilterMode() const override { return { m_MinFilter, m_MagFilter }; }
		void SetFilterMode(TextureFilterMode min, TextureFilterMode mag) override { m_MinFilter = min; m_MagFilter = mag; }

		int GetMipLevels() const override { return 1; }
		void GenerateMipmaps() override {}

		// CPU copy of the pixels (RGBA8). Empty for a texture loaded from a file:
		// the Vulkan backend decodes the file itself when this is empty.
		const std::vector<u8>& GetPixels() const { return m_Pixels; }

	private:
		fs::path m_Path;
		std::vector<u8> m_Pixels;
		u32 m_Width = 0;
		u32 m_Height = 0;
		TextureFormat m_Format = TextureFormat::RGBA8;
		TextureWrapMode m_WrapMode = TextureWrapMode::Repeat;
		TextureFilterMode m_MinFilter = TextureFilterMode::LinearMipmapLinear;
		TextureFilterMode m_MagFilter = TextureFilterMode::Linear;
	};

	// ---------------------------------------------------------------- Shader
	class NullShader : public Shader
	{
	public:
		explicit NullShader(const fs::path& filePath) : m_Path(filePath) {}
		NullShader(const std::string& vertexSrc, const std::string& fragmentSrc)
			: m_VertexSrc(vertexSrc), m_FragmentSrc(fragmentSrc) {}

		void Bind() const override {}
		void Unbind() const override {}

		void SetBool(const std::string& name, bool value) override {}
		void SetInt(const std::string& name, int value) override {}
		void SetFloat(const std::string& name, float value) override {}
		void SetVec2(const std::string& name, const glm::vec2& vector) override {}
		void SetVec3(const std::string& name, const glm::vec3& vector) override {}
		void SetVec4(const std::string& name, const glm::vec4& vector) override {}
		void SetMat4(const std::string& name, const glm::mat4& matrix) override {}

	protected:
		int GetUniformLocation(const std::string& name) override { return -1; }

	private:
		fs::path m_Path;
		std::string m_VertexSrc;
		std::string m_FragmentSrc;
	};

	// ---------------------------------------------------------------- Buffers
	class NullVertexBuffer : public VertexBuffer
	{
	public:
		explicit NullVertexBuffer(uint32_t size) : m_Data(size) {}
		NullVertexBuffer(const void* data, uint32_t size) : m_Data(size)
		{
			if (data && size) std::memcpy(m_Data.data(), data, size);
		}

		void Bind() const override {}
		void Unbind() const override {}
		void SetData(const void* data, uint32_t size) override
		{
			m_Data.resize(size);
			if (data && size) std::memcpy(m_Data.data(), data, size);
		}
		const BufferLayout& GetLayout() const override { return m_Layout; }
		void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

		// CPU copy of the vertices: the Vulkan renderer can upload it to a GPU buffer later
		const std::vector<u8>& GetData() const { return m_Data; }

	private:
		std::vector<u8> m_Data;
		BufferLayout m_Layout;
	};

	class NullIndexBuffer : public IndexBuffer
	{
	public:
		NullIndexBuffer(const uint32_t* indices, uint32_t count)
			: m_Indices(indices, indices + count) {}

		void Bind() const override {}
		void Unbind() const override {}
		uint32_t GetCount() const override { return (uint32_t)m_Indices.size(); }

		const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

	private:
		std::vector<uint32_t> m_Indices;
	};

	// ---------------------------------------------------------------- VertexArray / Mesh
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

	class NullMesh : public Mesh
	{
	public:
		NullMesh(const std::shared_ptr<VertexBuffer>& vb, const std::shared_ptr<IndexBuffer>& ib)
			: m_VertexBuffer(vb), m_IndexBuffer(ib) {}

		void Bind() const override {}
		void Draw() const override {}

		const std::shared_ptr<VertexBuffer>& GetVertexBuffer() const { return m_VertexBuffer; }
		const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }

	private:
		std::shared_ptr<VertexBuffer> m_VertexBuffer;
		std::shared_ptr<IndexBuffer> m_IndexBuffer;
	};
}
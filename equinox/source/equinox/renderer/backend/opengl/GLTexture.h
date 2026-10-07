#pragma once

#include "equinox/renderer/Texture.h"

namespace Equinox
{
    class GLTexture : public Texture
    {
    public:
        GLTexture(const fs::path& path);
        GLTexture(u32 width, u32 height, TextureFormat format, const void* data);
        ~GLTexture();

        void Bind(u32 slot = 0) const override;

        u32 GetWidth() const override { return m_Width; }
        u32 GetHeight() const override { return m_Height; }
        u32 GetRendererID() const override { return m_TextureID; }
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
        void SetWrapMode(TextureWrapMode mode) override;

        std::pair<TextureFilterMode, TextureFilterMode> GetFilterMode() const override
        {
            return { m_MinFilter, m_MagFilter };
        }
        void SetFilterMode(TextureFilterMode min, TextureFilterMode mag) override;

        int GetMipLevels() const override { return m_MipLevels; }
        void GenerateMipmaps() override;

    private:
        void LoadFromFile();
        void CreateInternal(uint32_t internalFormat);
        void CreateFromData(u32 width, u32 height, TextureFormat format, const void* data);

        uint32_t m_TextureID = 0;
        uint32_t m_Width = 0, m_Height = 0;
        fs::path m_Path;
        int m_MipLevels = 0;
        TextureFormat m_Format = TextureFormat::RGBA8;
        TextureWrapMode m_WrapMode = TextureWrapMode::Repeat;
        TextureFilterMode m_MinFilter = TextureFilterMode::Linear;
        TextureFilterMode m_MagFilter = TextureFilterMode::Linear;
    };
}

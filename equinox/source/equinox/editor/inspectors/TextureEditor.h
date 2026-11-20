#pragma once

#include "equinox/core/UUID.h"

namespace Equinox
{
    class Texture;

    class TextureEditor
    {
    public:
        void Draw(Texture& texture);

    private:
        UUID m_LastTextureUUID;
        int  m_WrapMode = 0;
        int  m_MinFilter = 0;
        int  m_MagFilter = 0;
        bool m_GenerateMipmaps = true;
    };
}
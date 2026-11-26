#pragma once

#include "equinox/core/UUID.h"

#include <string>

namespace Equinox
{
    class Shader;

    class ShaderEditor
    {
    public:
        void Draw(Shader& shader);

    private:
        UUID m_LastShaderUUID;
        std::string m_SourceCode;
    };
}

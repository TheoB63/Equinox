#pragma once

// OpenGL mirror of the Vulkan PoC shaders:
//     equinox/assets/shaders/triangle.vert
//     equinox/assets/shaders/triangle.frag
//
// The Vulkan versions are GLSL 450 using gl_VertexIndex (Vulkan-only) and are
// compiled to SPIR-V by ShaderCompiler. The OpenGL versions below are GLSL
// 460 core (desktop GL context, see WinWindow) and are compiled at runtime
// by GLShader (no SPIRV step).
//
// NOTE: GLSL forbids dynamic indexing of local arrays in vertex shaders, so
// the triangle data is selected with an if/else chain instead of a runtime
// array index. Same output as the Vulkan PoC: a centered RGB triangle.
namespace Equinox
{
    namespace GLTriangleShader
    {
        static const char* Vertex = R"GLSL(
#version 460 core

layout(location = 0) out vec3 fragColor;

void main() {
    vec2 pos;
    vec3 col;

    if (gl_VertexID == 0)
    {
        pos = vec2( 0.0, -0.5);
        col = vec3(1.0, 0.0, 0.0);
    }
    else if (gl_VertexID == 1)
    {
        pos = vec2( 0.5,  0.5);
        col = vec3(0.0, 1.0, 0.0);
    }
    else
    {
        pos = vec2(-0.5,  0.5);
        col = vec3(0.0, 0.0, 1.0);
    }

    gl_Position = vec4(pos, 0.0, 1.0);
    fragColor = col;
}
)GLSL";

        static const char* Fragment = R"GLSL(
#version 460 core

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
)GLSL";
    }
}

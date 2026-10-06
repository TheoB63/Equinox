#version 450

// ============================================================================
//  mesh.vert - the ONLY scene shader of the engine.
//
//  The same file is compiled twice:
//      glslc -V mesh.vert -o spv/mesh.vert.spv       -> Vulkan  (GfxVulkanBackend)
//      glslc -G mesh.vert -o spv/gl/mesh.vert.spv    -> OpenGL  (GfxOpenGLBackend,
//                                                       loaded through ARB_gl_spirv)
//  so Vulkan and OpenGL really run the SAME shader.
//
//  Rule for that to work: no push constant (OpenGL SPIR-V does not allow them),
//  and "set" must be 0. The per-object data is a std140 block (binding 2) bound
//  with a dynamic offset in Vulkan and with glBindBufferRange in OpenGL.
// ============================================================================

// ---- Vertex: position + normal + UV (engine Vertex, see renderer/Model.h)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

// ---- Per frame (binding 0): struct SceneUniform in graphics/GfxBackend.h
//      (the vertex stage only needs the view-projection matrix, but the block
//       must be declared identically in both stages: it is the same buffer)
layout(std140, set = 0, binding = 0) uniform SceneUBO
{
    mat4 viewProj;
    vec4 cameraPos;    // xyz = world position of the camera
    vec4 lightDir;     // xyz = fallback direction TOWARDS the light
    vec4 lightColor;   // rgb = fallback radiance
    vec4 params;       // x = ambient, w = time (seconds)
    vec4 post0;        // x = exposure, y = contrast,  z = saturation, w = tone mapping operator
    vec4 post1;        // x = vignette amount, y = vignette hardness, z = grain, w = chromatic aberration
    vec4 post2;        // rgb = shadows balance,    a = post-process enabled
    vec4 post3;        // rgb = midtones balance
    vec4 post4;        // rgb = highlights balance
    vec4 viewport;     // xy = size of the render target (for the screen-space effects)
} scene;

// ---- Per object (binding 2, selected by a dynamic offset):
//      struct ObjectUniform in graphics/GfxBackend.h
layout(std140, set = 0, binding = 2) uniform ObjectUBO
{
    mat4 model;
    vec4 color;        // rgb = albedo,   a = alpha
    vec4 params;       // x = roughness,  y = uvScale, z = useTexture, w = metallic
    vec4 lightDir;     // xyz = direction TOWARDS the light of this object, w = 1 if the object is lit
    vec4 lightColor;   // rgb = radiance accumulated from the ECS lights (see GfxScene)
    vec4 emissive;     // rgb = emissive colour of the material
} obj;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vUV;
layout(location = 3) out vec2 vScreenUV;   // 0..1 across the render target (post-process)

void main()
{
    vec4 world = obj.model * vec4(inPosition, 1.0);
    vWorldPos = world.xyz;

    // Entities may have a non-uniform scale, so the normals need the
    // inverse transpose of the model matrix (the old version only worked
    // because the demo cubes had a uniform scale).
    mat3 normalMatrix = transpose(inverse(mat3(obj.model)));
    vNormal = normalize(normalMatrix * inNormal);

    vUV = inUV * obj.params.y;
    gl_Position = scene.viewProj * world;

    // Screen-space UV used by the vignette / grain / chromatic aberration.
    // Vulkan renders with a flipped viewport (negative height), so we flip Y
    // back to keep "0 at the bottom" in both APIs, like OpenGL.
    vec2 ndc = gl_Position.xy / max(gl_Position.w, 0.0001);
    vScreenUV = ndc * 0.5 + 0.5;
}

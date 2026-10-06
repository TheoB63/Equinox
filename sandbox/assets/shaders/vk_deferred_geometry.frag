#version 450

layout(location = 0) in vec3 vWorldPosition;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec2 vUV;

layout(std140, set = 0, binding = 1) uniform ObjectUBO
{
    mat4 model;
    vec4 color;
    vec4 params;       // roughness, UV scale, use texture, metallic
    vec4 lightDir;
    vec4 lightColor;
    vec4 emissive;
} objectData;

layout(set = 1, binding = 0) uniform sampler2D baseTexture;

layout(location = 0) out vec4 outPositionRoughness;
layout(location = 1) out vec4 outNormalMetallic;
layout(location = 2) out vec4 outAlbedoAlpha;
layout(location = 3) out vec4 outEmissive;

void main()
{
    vec4 texel = objectData.params.z > 0.5 ? texture(baseTexture, vUV) : vec4(1.0);
    vec4 albedo = objectData.color * texel;

    // A pragmatic cutout fallback until alphaCutoff is carried by DrawItem.
    if (albedo.a < 0.5)
        discard;

    outPositionRoughness = vec4(vWorldPosition, clamp(objectData.params.x, 0.045, 1.0));
    outNormalMetallic = vec4(normalize(vWorldNormal), clamp(objectData.params.w, 0.0, 1.0));
    outAlbedoAlpha = albedo;
    outEmissive = vec4(max(objectData.emissive.rgb, vec3(0.0)), 1.0);
}

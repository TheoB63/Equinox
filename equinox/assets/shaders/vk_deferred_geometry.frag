#version 450

layout(location = 0) in vec3 vWorldPosition;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec2 vUV0;
layout(location = 3) in vec2 vUV1;

layout(std140, set = 0, binding = 1) uniform ObjectUBO
{
    mat4 model;
    vec4 color;
    vec4 params;       // roughness, UV scale, has diffuse texture, metallic
    vec4 lightDir;
    vec4 lightColor;
    vec4 emissive;
    vec4 alpha;        // cutoff, has separate alpha map, render mode, alpha-from-diffuse
    vec4 uvSets;       // diffuse UV index, alpha UV index, reserved, reserved
} objectData;

layout(set = 1, binding = 0) uniform sampler2D baseTexture;
layout(set = 2, binding = 0) uniform sampler2D alphaTexture;

layout(location = 0) out vec4 outPositionRoughness;
layout(location = 1) out vec4 outNormalMetallic;
layout(location = 2) out vec4 outAlbedoAlpha;
layout(location = 3) out vec4 outEmissive;

vec2 selectUV(float index)
{
    return index < 0.5 ? vUV0 : vUV1;
}

void main()
{
    vec2 diffuseUV = selectUV(objectData.uvSets.x);
    vec4 texel = objectData.params.z > 0.5 ? texture(baseTexture, diffuseUV) : vec4(1.0);
    vec3 albedo = objectData.color.rgb * texel.rgb;

    // Match EquinoxDeferredGeo.glsl exactly for the alpha source.
    float opacity = objectData.color.a;
    if (objectData.alpha.w > 0.5)
        opacity *= texel.a;
    else if (objectData.alpha.y > 0.5)
        opacity *= texture(alphaTexture, selectUV(objectData.uvSets.y)).r;

    int renderMode = int(objectData.alpha.z + 0.5);
    if (renderMode == 1 && opacity < objectData.alpha.x)
        discard;
    if (renderMode == 0)
        opacity = 1.0;

    outPositionRoughness = vec4(vWorldPosition, clamp(objectData.params.x, 0.045, 1.0));
    outNormalMetallic = vec4(normalize(vWorldNormal), clamp(objectData.params.w, 0.0, 1.0));
    outAlbedoAlpha = vec4(albedo, opacity);
    outEmissive = vec4(max(objectData.emissive.rgb, vec3(0.0)), 1.0);
}

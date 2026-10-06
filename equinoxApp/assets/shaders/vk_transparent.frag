#version 450

layout(location = 0) in vec3 vWorldPosition;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec2 vUV;

layout(std140, set = 0, binding = 0) uniform SceneUBO
{
    mat4 view;
    mat4 projection;
    mat4 viewProj;
    vec4 cameraPos;
    vec4 lightDirAmbient;
    vec4 lightColor;
    vec4 viewportTime;
    vec4 ssao;
    vec4 bloom;
    vec4 post0;
    vec4 post1;
    vec4 post2;
    vec4 post3;
    vec4 post4;
    vec4 clearColor;
} scene;

layout(std140, set = 0, binding = 1) uniform ObjectUBO
{
    mat4 model;
    vec4 color;
    vec4 params;
    vec4 lightDir;
    vec4 lightColor;
    vec4 emissive;
} objectData;

layout(set = 1, binding = 0) uniform sampler2D baseTexture;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a2 = pow(roughness, 4.0);
    float nDotH = max(dot(N, H), 0.0);
    float d = nDotH * nDotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 0.000001);
}
float GeometrySchlickGGX(float nDotV, float roughness)
{
    float k = pow(roughness + 1.0, 2.0) / 8.0;
    return nDotV / max(nDotV * (1.0 - k) + k, 0.000001);
}
vec3 FresnelSchlick(float value, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - value, 0.0, 1.0), 5.0);
}

void main()
{
    vec4 texel = objectData.params.z > 0.5 ? texture(baseTexture, vUV) : vec4(1.0);
    vec4 base = objectData.color * texel;
    if (base.a <= 0.001) discard;

    vec3 N = normalize(vWorldNormal);
    vec3 V = normalize(scene.cameraPos.xyz - vWorldPosition);
    vec3 L = objectData.lightDir.w > 0.5 ? normalize(objectData.lightDir.xyz) : normalize(scene.lightDirAmbient.xyz);
    vec3 radiance = objectData.lightDir.w > 0.5 ? objectData.lightColor.rgb : scene.lightColor.rgb;
    vec3 H = normalize(V + L);
    float roughness = clamp(objectData.params.x, 0.045, 1.0);
    float metallic = clamp(objectData.params.w, 0.0, 1.0);
    vec3 f0 = mix(vec3(0.04), base.rgb, metallic);
    float nDotL = max(dot(N, L), 0.0);
    float nDotV = max(dot(N, V), 0.0);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), f0);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
    vec3 specular = D * G * F / max(4.0 * nDotV * nDotL, 0.0001);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 direct = (kD * base.rgb / PI + specular) * radiance * nDotL;
    vec3 ambient = base.rgb * scene.lightDirAmbient.w;
    outColor = vec4(ambient + direct + objectData.emissive.rgb, base.a);
}

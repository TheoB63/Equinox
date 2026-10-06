#version 450

layout(location = 0) in vec2 vUV;
layout(set = 0, binding = 0) uniform sampler2D gPositionRoughness;
layout(set = 0, binding = 1) uniform sampler2D gNormalMetallic;
layout(set = 0, binding = 2) uniform sampler2D gAlbedoAlpha;
layout(set = 0, binding = 3) uniform sampler2D gEmissive;
layout(set = 0, binding = 4) uniform sampler2D ssaoTexture;

#define MAX_DIR_LIGHTS 4
#define MAX_POINT_LIGHTS 16

layout(std140, set = 0, binding = 5) uniform SceneUBO
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
    vec4 lightCounts;
    vec4 dirDirections[MAX_DIR_LIGHTS];
    vec4 dirColorsIntensity[MAX_DIR_LIGHTS];
    vec4 pointPositionsRange[MAX_POINT_LIGHTS];
    vec4 pointColorsIntensity[MAX_POINT_LIGHTS];
} scene;

layout(location = 0) out vec4 outHdrColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float nDotH = max(dot(N, H), 0.0);
    float denominator = nDotH * nDotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denominator * denominator, 0.000001);
}

float GeometrySchlickGGX(float nDotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return nDotV / max(nDotV * (1.0 - k) + k, 0.000001);
}

vec3 FresnelSchlick(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalculateLight(vec3 L, vec3 radiance, vec3 V, vec3 N,
                    vec3 albedo, float metallic, float roughness)
{
    float nDotL = max(dot(N, L), 0.0);
    float nDotV = max(dot(N, V), 0.0);
    if (nDotL <= 0.0 || nDotV <= 0.0)
        return vec3(0.0);

    vec3 H = normalize(V + L);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), f0);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
    vec3 specular = D * G * F / max(4.0 * nDotV * nDotL, 0.0001);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    return (kD * albedo / PI + specular) * radiance * nDotL;
}

void main()
{
    vec4 positionRoughness = texture(gPositionRoughness, vUV);
    vec4 normalMetallic = texture(gNormalMetallic, vUV);
    if (dot(normalMetallic.xyz, normalMetallic.xyz) < 0.01)
    {
        outHdrColor = scene.clearColor;
        return;
    }

    vec3 worldPosition = positionRoughness.xyz;
    float roughness = clamp(positionRoughness.w, 0.045, 1.0);
    vec3 N = normalize(normalMetallic.xyz);
    float metallic = clamp(normalMetallic.w, 0.0, 1.0);
    vec3 albedo = texture(gAlbedoAlpha, vUV).rgb;
    vec3 emissive = texture(gEmissive, vUV).rgb;
    vec3 V = normalize(scene.cameraPos.xyz - worldPosition);
    vec3 direct = vec3(0.0);

    int dirCount = clamp(int(scene.lightCounts.x + 0.5), 0, MAX_DIR_LIGHTS);
    for (int i = 0; i < dirCount; ++i)
    {
        vec3 L = normalize(-scene.dirDirections[i].xyz);
        vec3 radiance = scene.dirColorsIntensity[i].rgb * scene.dirColorsIntensity[i].a;
        direct += CalculateLight(L, radiance, V, N, albedo, metallic, roughness);
    }

    int pointCount = clamp(int(scene.lightCounts.y + 0.5), 0, MAX_POINT_LIGHTS);
    for (int i = 0; i < pointCount; ++i)
    {
        vec3 toLight = scene.pointPositionsRange[i].xyz - worldPosition;
        float distanceToLight = length(toLight);
        float range = max(scene.pointPositionsRange[i].a, 0.0001);
        float scaledDistance = distanceToLight / range;
        if (scaledDistance > 1.0)
            continue;

        vec3 L = distanceToLight > 0.0001 ? toLight / distanceToLight : vec3(0.0, 1.0, 0.0);
        // Keep the current OpenGL shader's range-window attenuation formula.
        float attenuation = clamp(1.0 - pow(scaledDistance, 4.0), 0.0, 1.0);
        vec3 radiance = scene.pointColorsIntensity[i].rgb *
                        scene.pointColorsIntensity[i].a * attenuation;
        direct += CalculateLight(L, radiance, V, N, albedo, metallic, roughness);
    }

    float rawAo = texture(ssaoTexture, vUV).r;
    float ao = mix(1.0, rawAo, clamp(scene.ssao.z, 0.0, 1.0));
    vec3 ambient = albedo * scene.lightDirAmbient.w * ao;
    outHdrColor = vec4(ambient + direct + emissive, 1.0);
}

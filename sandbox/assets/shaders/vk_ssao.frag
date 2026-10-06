#version 450

layout(location = 0) in vec2 vUV;
layout(set = 0, binding = 0) uniform sampler2D gPositionRoughness;
layout(set = 0, binding = 1) uniform sampler2D gNormalMetallic;

layout(std140, set = 0, binding = 2) uniform SceneUBO
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

layout(std140, set = 0, binding = 3) uniform KernelUBO
{
    vec4 samples[64];
} kernel;

layout(location = 0) out float outOcclusion;

float Hash(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec3 worldPosition = texture(gPositionRoughness, vUV).xyz;
    vec3 normal = texture(gNormalMetallic, vUV).xyz;
    if (dot(normal, normal) < 0.01)
    {
        outOcclusion = 1.0;
        return;
    }
    normal = normalize(normal);

    vec2 pixel = vUV * scene.viewportTime.xy;
    vec3 randomVector = normalize(vec3(Hash(pixel), Hash(pixel + 17.17), 0.0) * 2.0 - vec3(1.0, 1.0, 0.0));
    vec3 tangent = randomVector - normal * dot(randomVector, normal);
    if (dot(tangent, tangent) < 0.001)
        tangent = abs(normal.z) < 0.999 ? normalize(cross(normal, vec3(0.0, 0.0, 1.0))) : vec3(1.0, 0.0, 0.0);
    else
        tangent = normalize(tangent);
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    float radius = max(scene.ssao.x, 0.001);
    float bias = max(scene.ssao.y, 0.0);
    int sampleCount = clamp(int(scene.ssao.w + 0.5), 1, 64);
    float fragmentViewZ = (scene.view * vec4(worldPosition, 1.0)).z;
    float occlusion = 0.0;

    for (int i = 0; i < sampleCount; ++i)
    {
        vec3 samplePosition = worldPosition + (tbn * kernel.samples[i].xyz) * radius;
        vec4 clip = scene.viewProj * vec4(samplePosition, 1.0);
        if (clip.w <= 0.0)
            continue;

        vec2 sampleUV = clip.xy / clip.w * 0.5 + 0.5;
        // Geometry uses a negative Vulkan viewport, therefore NDC +Y is image top.
        sampleUV.y = 1.0 - sampleUV.y;
        if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
            continue;

        vec3 sampledNormal = texture(gNormalMetallic, sampleUV).xyz;
        if (dot(sampledNormal, sampledNormal) < 0.01)
            continue;

        vec3 sampledPosition = texture(gPositionRoughness, sampleUV).xyz;
        float sampledViewZ = (scene.view * vec4(sampledPosition, 1.0)).z;
        float targetViewZ = (scene.view * vec4(samplePosition, 1.0)).z;
        float rangeWeight = smoothstep(0.0, 1.0, radius / max(abs(fragmentViewZ - sampledViewZ), 0.0001));
        occlusion += (sampledViewZ >= targetViewZ + bias ? 1.0 : 0.0) * rangeWeight;
    }

    outOcclusion = clamp(1.0 - occlusion / float(sampleCount), 0.0, 1.0);
}

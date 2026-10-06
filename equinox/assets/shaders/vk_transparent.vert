#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV0;
layout(location = 3) in vec2 inUV1;
layout(location = 4) in vec3 inTangent;

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
    vec4 alpha;
    vec4 uvSets;
} objectData;

layout(location = 0) out vec3 vWorldPosition;
layout(location = 1) out vec3 vWorldNormal;
layout(location = 2) out vec2 vUV0;
layout(location = 3) out vec2 vUV1;

void main()
{
    vec4 worldPosition = objectData.model * vec4(inPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vWorldNormal = normalize(transpose(inverse(mat3(objectData.model))) * inNormal);
    vUV0 = inUV0 * objectData.params.y;
    vUV1 = inUV1 * objectData.params.y;
    gl_Position = scene.viewProj * worldPosition;
}

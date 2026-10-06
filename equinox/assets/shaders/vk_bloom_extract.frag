#version 450

layout(location = 0) in vec2 vUV;
layout(set = 0, binding = 0) uniform sampler2D hdrScene;

layout(std140, set = 0, binding = 1) uniform SceneUBO
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

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 color = texture(hdrScene, vUV).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float mask = smoothstep(scene.bloom.x, scene.bloom.x + 0.1, brightness);
    if (mask == 0.0)
        discard;
    outColor = vec4(color * mask, 1.0);
}

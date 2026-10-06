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
    float brightness = max(color.r, max(color.g, color.b));
    float threshold = max(scene.bloom.x, 0.0);
    float knee = max(threshold * 0.5, 0.0001);
    float soft = clamp((brightness - threshold + knee) / (2.0 * knee), 0.0, 1.0);
    float contribution = max(brightness - threshold, 0.0) + soft * soft * knee;
    contribution /= max(brightness, 0.0001);
    outColor = vec4(color * contribution, 1.0);
}

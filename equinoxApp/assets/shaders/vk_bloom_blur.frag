#version 450

layout(location = 0) in vec2 vUV;
layout(set = 0, binding = 0) uniform sampler2D inputImage;

layout(push_constant) uniform BlurPush
{
    vec2 texelSize;
    int horizontal;
    float strength;
} pushData;

layout(location = 0) out vec4 outColor;

void main()
{
    const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec2 axis = pushData.horizontal != 0 ? vec2(pushData.texelSize.x, 0.0) : vec2(0.0, pushData.texelSize.y);

    vec3 color = texture(inputImage, vUV).rgb * weights[0];
    for (int i = 1; i < 5; ++i)
    {
        vec2 offset = axis * float(i) * max(pushData.strength, 0.01);
        color += texture(inputImage, vUV + offset).rgb * weights[i];
        color += texture(inputImage, vUV - offset).rgb * weights[i];
    }
    outColor = vec4(color, 1.0);
}

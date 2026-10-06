#version 450

layout(location = 0) in vec2 vUV;
layout(set = 0, binding = 0) uniform sampler2D inputOcclusion;
layout(location = 0) out float outOcclusion;

void main()
{
    vec2 texel = 1.0 / vec2(textureSize(inputOcclusion, 0));
    float result = 0.0;
    float weight = 0.0;

    // Small separable-quality box filter. A depth-aware bilateral blur is the
    // natural next improvement when depth is also bound to this pass.
    for (int y = -2; y <= 2; ++y)
    {
        for (int x = -2; x <= 2; ++x)
        {
            float w = 1.0 / (1.0 + float(x * x + y * y));
            result += texture(inputOcclusion, vUV + vec2(x, y) * texel).r * w;
            weight += w;
        }
    }
    outOcclusion = result / max(weight, 0.0001);
}

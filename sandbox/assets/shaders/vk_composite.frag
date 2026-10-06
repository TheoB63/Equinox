#version 450

layout(location = 0) in vec2 vUV;
layout(set = 0, binding = 0) uniform sampler2D hdrScene;
layout(set = 0, binding = 1) uniform sampler2D bloomImage;

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

layout(location = 0) out vec4 outColor;

float Hash(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 SampleScene(vec2 uv)
{
    float aberration = max(scene.post1.w, 0.0);
    if (aberration <= 0.00001)
        return texture(hdrScene, uv).rgb;

    vec2 direction = (uv - 0.5) * aberration;
    return vec3(
        texture(hdrScene, uv + direction).r,
        texture(hdrScene, uv).g,
        texture(hdrScene, uv - direction).b);
}

vec3 ToneMap(vec3 color, int operation)
{
    float exposure = max(scene.post0.x, 0.0);
    color *= exposure;

    if (operation == 0) return color;
    if (operation == 1) return color / (1.0 + color);
    if (operation == 2)
    {
        const float whitePoint = 4.0;
        return color * (1.0 + color / (whitePoint * whitePoint)) / (1.0 + color);
    }
    if (operation == 3)
    {
        const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
        return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
    }
    if (operation == 4)
    {
        vec3 x = max(vec3(0.0), color - 0.004);
        return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
    }

    const float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;
    return ((color * (A * color + C * B) + D * E) /
           (color * (A * color + B) + D * F)) - E / F;
}

vec3 Sharpen(vec2 uv, vec3 center)
{
    float amount = max(scene.post4.w, 0.0);
    if (amount <= 0.0001) return center;

    vec2 texel = 1.0 / scene.viewportTime.xy;
    vec3 neighbours = texture(hdrScene, uv + vec2(texel.x, 0.0)).rgb +
                      texture(hdrScene, uv - vec2(texel.x, 0.0)).rgb +
                      texture(hdrScene, uv + vec2(0.0, texel.y)).rgb +
                      texture(hdrScene, uv - vec2(0.0, texel.y)).rgb;
    return max(center * (1.0 + 4.0 * amount) - neighbours * amount, vec3(0.0));
}

void main()
{
    bool enabled = scene.viewportTime.w > 0.5;
    vec3 color = SampleScene(vUV);

    if (enabled)
    {
        color = Sharpen(vUV, color);
        color += texture(bloomImage, vUV).rgb * max(scene.bloom.y, 0.0);
        color = ToneMap(color, int(scene.post0.w + 0.5));

        float contrast = max(scene.post0.y, 0.0001);
        color = pow(max(color, vec3(0.0)), vec3(1.0 / contrast));
        float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
        color = mix(vec3(luminance), color, max(scene.post0.z, 0.0));

        // Smooth luminance masks keep the three colour-balance controls local.
        float shadowMask = 1.0 - smoothstep(0.0, 0.5, luminance);
        float highlightMask = smoothstep(0.5, 1.0, luminance);
        float midtoneMask = max(1.0 - shadowMask - highlightMask, 0.0);
        color *= mix(vec3(1.0), scene.post2.rgb, shadowMask);
        color *= mix(vec3(1.0), scene.post3.rgb, midtoneMask);
        color *= mix(vec3(1.0), scene.post4.rgb, highlightMask);

        float grain = (Hash(vUV * scene.viewportTime.xy + scene.viewportTime.z) - 0.5) * scene.post1.z;
        color += grain;

        vec2 centered = vUV - 0.5;
        float distanceFromCenter = length(centered);
        float radius = mix(0.7071, 0.0, clamp(scene.post1.x, 0.0, 1.0));
        float softness = mix(0.5, 0.01, clamp(scene.post1.y, 0.0, 1.0));
        float vignette = smoothstep(radius, radius + softness, distanceFromCenter);
        color *= 1.0 - vignette;
    }
    else
    {
        color = clamp(color, 0.0, 1.0);
    }

    // Final target is UNORM, not sRGB: encode display gamma explicitly.
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
    outColor = vec4(color, 1.0);
}

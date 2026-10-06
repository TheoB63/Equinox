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
    vec4 bloom;        // threshold, strength, passes, debug view
    vec4 post0;        // exposure, contrast, saturation, tone mapper
    vec4 post1;        // vignette amount/hardness, grain, aberration
    vec4 post2;        // shadow balance
    vec4 post3;        // midtone balance
    vec4 post4;        // highlight balance, sharpness in w
    vec4 clearColor;
} scene;

layout(set = 0, binding = 3) uniform sampler2D gPositionRoughness;
layout(set = 0, binding = 4) uniform sampler2D gNormalMetallic;
layout(set = 0, binding = 5) uniform sampler2D gAlbedoAlpha;
layout(set = 0, binding = 6) uniform sampler2D gEmissive;
layout(set = 0, binding = 7) uniform sampler2D ssaoImage;

layout(location = 0) out vec4 outColor;

float Rand(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 ToneMap(vec3 color, int operation)
{
    float exposure = scene.post0.x;
    if (operation == 0) return color * exposure;

    color *= exposure;
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

vec3 DebugView(int mode)
{
    vec4 positionRoughness = texture(gPositionRoughness, vUV);
    vec4 normalMetallic = texture(gNormalMetallic, vUV);
    vec4 albedoAlpha = texture(gAlbedoAlpha, vUV);

    if (mode == 1) return albedoAlpha.rgb;
    if (mode == 2) return vec3(normalMetallic.a);
    if (mode == 3) return vec3(positionRoughness.a);
    if (mode == 4) return normalMetallic.rgb * 0.5 + 0.5;
    if (mode == 5) return texture(gEmissive, vUV).rgb;
    if (mode == 6) return vec3(albedoAlpha.a);
    if (mode == 7)
    {
        vec3 p = positionRoughness.xyz;
        float magnitude = max(length(p), 0.0001);
        return p / magnitude * 0.5 + 0.5;
    }
    if (mode == 8)
        return vec3(normalMetallic.a, positionRoughness.a, texture(ssaoImage, vUV).r);
    return vec3(texture(ssaoImage, vUV).r);
}

void main()
{
    int debugMode = int(scene.bloom.w + 0.5);
    if (debugMode > 0)
    {
        outColor = vec4(clamp(DebugView(debugMode), 0.0, 1.0), 1.0);
        return;
    }

    bool enabled = scene.viewportTime.w > 0.5;
    vec3 color = texture(hdrScene, vUV).rgb;

    if (enabled)
    {
        // Keep the same operation order and formulas as EquinoxPostProcess.glsl.
        color += texture(bloomImage, vUV).rgb * scene.bloom.y;
        color = ToneMap(color, int(scene.post0.w + 0.5));
        color = pow(color, vec3(1.0 / scene.post0.y));
        float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
        color = mix(vec3(luminance), color, scene.post0.z);

        color = mix(color, color * scene.post2.rgb, 0.33);
        color = mix(color, color * scene.post3.rgb, 0.33);
        color = mix(color, color * scene.post4.rgb, 0.33);

        vec2 texel = 1.0 / vec2(textureSize(hdrScene, 0));
        vec3 blurred = (
            texture(hdrScene, vUV + vec2(texel.x, 0.0)).rgb +
            texture(hdrScene, vUV - vec2(texel.x, 0.0)).rgb +
            texture(hdrScene, vUV + vec2(0.0, texel.y)).rgb +
            texture(hdrScene, vUV - vec2(0.0, texel.y)).rgb) * 0.25;
        color = color + (color - blurred) * scene.post4.w;

        float offset = scene.post1.w;
        vec2 rUV = clamp(vUV + vec2(offset, 0.0), 0.0, 1.0);
        vec2 gUV = vUV;
        vec2 bUV = clamp(vUV - vec2(offset, 0.0), 0.0, 1.0);
        color = vec3(
            texture(hdrScene, rUV).r * color.r,
            texture(hdrScene, gUV).g * color.g,
            texture(hdrScene, bUV).b * color.b);

        color += (Rand(vUV * scene.viewportTime.z) - 0.5) * scene.post1.z;

        vec2 centered = vUV - 0.5;
        float radius = mix(0.7071, 0.0, scene.post1.x);
        float smoothness = mix(0.5, 0.01, scene.post1.y);
        float vignette = smoothstep(radius, radius + smoothness, length(centered));
        color *= 1.0 - vignette;

        color = pow(color, vec3(1.0 / 2.2));
    }
    else
    {
        color = clamp(color, 0.0, 1.0);
    }

    outColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}

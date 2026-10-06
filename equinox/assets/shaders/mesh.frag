#version 450

// ============================================================================
//  mesh.frag - the ONLY scene shader of the engine (see mesh.vert for the
//  "one source, two SPIR-V targets" explanation).
//
//  It replaces the old EquinoxForwardLight / EquinoxDeferred* shaders of the
//  legacy deferred pipeline (removed) AND applies the post-process effects at
//  the end, so that BOTH APIs (Vulkan and OpenGL) show exactly the same image:
//
//    - PBR lighting          : Cook-Torrance (GGX + Smith + Schlick)
//                              albedo / metallic / roughness / emissive come
//                              from the engine Material of the entity,
//    - lights                : one light per object, accumulated on the CPU by
//                              GfxScene from every <DirectionalLight> and
//                              <PointLight> of the ECS (fallback: the scene
//                              settings when the hierarchy has no light),
//    - post-process          : exposure, tone mapping, contrast, saturation,
//                              colour balance (shadows/midtones/highlights),
//                              vignette, film grain, chromatic aberration.
//                              All the parameters come from the "Render" panel.
// ============================================================================

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec2 vScreenUV;

// ---- Per frame (binding 0)
layout(std140, set = 0, binding = 0) uniform SceneUBO
{
    mat4 viewProj;
    vec4 cameraPos;    // xyz = world position of the camera
    vec4 lightDir;     // xyz = fallback direction TOWARDS the light
    vec4 lightColor;   // rgb = fallback radiance
    vec4 params;       // x = ambient, w = time (seconds)
    vec4 post0;        // x = exposure, y = contrast,  z = saturation, w = tone mapping operator
    vec4 post1;        // x = vignette amount, y = vignette hardness, z = grain, w = chromatic aberration
    vec4 post2;        // rgb = shadows balance,    a = post-process enabled
    vec4 post3;        // rgb = midtones balance
    vec4 post4;        // rgb = highlights balance
    vec4 viewport;     // xy = size of the render target
} scene;

// ---- Diffuse / albedo texture (binding 1)
layout(set = 0, binding = 1) uniform sampler2D baseTexture;

// ---- Per object (binding 2, dynamic offset)
layout(std140, set = 0, binding = 2) uniform ObjectUBO
{
    mat4 model;
    vec4 color;        // rgb = albedo,   a = alpha
    vec4 params;       // x = roughness,  y = uvScale, z = useTexture, w = metallic
    vec4 lightDir;     // xyz = direction TOWARDS the light, w = 1 if the object is lit
    vec4 lightColor;   // rgb = radiance accumulated from the ECS lights
    vec4 emissive;     // rgb = emissive colour of the material
} obj;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

// ============================================================================
//  PBR
// ============================================================================
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 0.000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / max(NdotV * (1.0 - k) + k, 0.000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) *
           GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ============================================================================
//  Albedo (with an optional chromatic aberration: the three channels are read
//  at three slightly different UVs, like a real post-process pass would do)
// ============================================================================
vec3 SampleAlbedo(vec2 uv, float aberration)
{
    if (obj.params.z < 0.5)
        return vec3(1.0);

    if (aberration <= 0.00001)
        return texture(baseTexture, uv).rgb;

    vec2 center = vec2(0.5);
    vec2 dir = (uv - center) * aberration;
    vec3 c;
    c.r = texture(baseTexture, uv + dir).r;
    c.g = texture(baseTexture, uv).g;
    c.b = texture(baseTexture, uv - dir).b;
    return c;
}

// ============================================================================
//  Tone mapping (operator = index of Gfx::ToneMapOperator, see GfxBackend.h)
// ============================================================================
vec3 ToneMap(vec3 x, int op)
{
    if (op == 1)                       // Reinhard
    {
        return x / (x + vec3(1.0));
    }
    else if (op == 2)                  // Modified Reinhard
    {
        const float white = 4.0;
        return (x * (1.0 + x / (white * white))) / (1.0 + x);
    }
    else if (op == 3)                  // ACES (Narkowicz)
    {
        const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
        return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
    }
    else if (op == 4)                  // Filmic (Uncharted 2 style curve)
    {
        const float A = 0.22, B = 0.30, C = 0.10, D = 0.20, E = 0.01, F = 0.30;
        vec3 v = max(x - 0.004, vec3(0.0));
        return ((v * (A * v + C * B) + D * E) / (v * (A * v + B) + D * F)) - E / F;
    }
    else if (op == 5)                  // Uncharted 2
    {
        const float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;
        vec3 v = max(x - 0.004, vec3(0.0));
        return ((v * (A * v + C * B) + D * E) / (v * (A * v + B) + D * F)) - E / F;
    }
    return clamp(x, 0.0, 1.0);   // "Linear": only clamp to a displayable range
}

// ============================================================================
//  Post-process
//
//  CES FONCTIONS SONT LA COPIE DE equinoxApp/assets/shaders/EquinoxPostProcess.glsl
//  (memes operateurs, memes constantes, meme ordre) : le panneau "Render" donne
//  donc le meme resultat en Vulkan qu'en OpenGL pour les effets par pixel.
//
//  Ce qui, en OpenGL, vient d'une passe separee n'est pas ici :
//      - bloom       : extraction + flou ping-pong  -> passe bloomStrength/BloomPasses
//      - sharpness   : echantillonne les pixels voisins de l'image finale
//      - aberration  : dans EquinoxPostProcess elle lit l'image de scene ; en
//                      rendu direct il n'y a pas d'image de scene a relire, elle
//                      est donc appliquee sur la texture du materiau (voir
//                      SampleAlbedo), ce qui donne le meme lisere de couleur.
// ============================================================================
float Rand(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 ToneMapLinear(vec3 color, float exposure)     { return color * exposure; }

vec3 ToneMapReinhard(vec3 color, float exposure)   { color *= exposure; return color / (1.0 + color); }

vec3 ToneMapReinhardMod(vec3 color, float exposure)
{
    const float L_white = 4.0;
    color *= exposure;
    return color * (1.0 + color / (L_white * L_white)) / (1.0 + color);
}

vec3 ToneMapACES(vec3 color, float exposure)
{
    color *= exposure;
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 ToneMapFilmic(vec3 color, float exposure)
{
    color *= exposure;
    vec3 x = max(vec3(0.0), color - 0.004);
    return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
}

vec3 ToneMapUncharted2(vec3 color, float exposure)
{
    color *= exposure;
    const float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;
    return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

// operator = index de Gfx::ToneMapOperator (GfxBackend.h) = meme ordre que le
// combo "Operator" du panneau Render et que EquinoxPostProcess.glsl.
vec3 ApplyToneMapping(vec3 color)
{
    const int op = int(scene.post0.w + 0.5);
    const float exposure = max(scene.post0.x, 0.0);

    if (op == 0)      color = ToneMapLinear(color, exposure);
    else if (op == 1) color = ToneMapReinhard(color, exposure);
    else if (op == 2) color = ToneMapReinhardMod(color, exposure);
    else if (op == 3) color = ToneMapACES(color, exposure);
    else if (op == 4) color = ToneMapFilmic(color, exposure);
    else if (op == 5) color = ToneMapUncharted2(color, exposure);
    else              color = ToneMapACES(color, exposure);

    // Contrast (pow, comme ApplyContrast de EquinoxPostProcess) et saturation
    const float contrast = max(scene.post0.y, 0.0001);
    color = pow(max(color, vec3(0.0)), vec3(1.0 / contrast));

    const float saturation = max(scene.post0.z, 0.0);
    const vec3 luminance = vec3(0.2126, 0.7152, 0.0722);
    float lum = dot(color, luminance);
    return mix(vec3(lum), color, saturation);
}

vec3 ApplyColorBalance(vec3 color)
{
    color = mix(color, color * scene.post2.rgb, 0.33);   // ombres
    color = mix(color, color * scene.post3.rgb, 0.33);   // tons moyens
    color = mix(color, color * scene.post4.rgb, 0.33);   // hautes lumieres
    return color;
}

vec3 ApplyVignette(vec3 color)
{
    const float amount = clamp(scene.post1.x, 0.0, 1.0);
    const float hardness = clamp(scene.post1.y, 0.0, 1.0);

    vec2 centeredUV = vScreenUV - 0.5;
    float dist = length(centeredUV);
    float radius = mix(0.7071, 0.0, amount);
    float smoothness = mix(0.5, 0.01, hardness);
    float vignette = smoothstep(radius, radius + smoothness, dist);
    return color * (1.0 - vignette);
}

vec3 ApplyGrain(vec3 color)
{
    float g = (Rand(vScreenUV * scene.params.w) - 0.5) * scene.post1.z;
    return color + g;
}

vec3 ApplyPostProcess(vec3 color)
{
    // post2.w = 1 quand le post-traitement est actif (bouton "No Post-Processing")
    if (scene.post2.w < 0.5)
        return color;

    color = ApplyToneMapping(color);
    color = ApplyColorBalance(color);
    // sharpness : passe separee, OpenGL uniquement
    color = ApplyGrain(color);
    color = ApplyVignette(color);
    return clamp(color, 0.0, 1.0);
}

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(scene.cameraPos.xyz - vWorldPos);

    // ---- Material ----
    float aberration = (scene.post2.w < 0.5) ? 0.0 : clamp(scene.post1.w, 0.0, 0.05);
    vec3 albedo = obj.color.rgb * SampleAlbedo(vUV, aberration);
    vec3 emissive = obj.emissive.rgb;

    float metallic = clamp(obj.params.w, 0.0, 1.0);
    float roughness = clamp(obj.params.x, 0.045, 1.0);
    float ambient = scene.params.x;

    // ---- Light (direction + radiance computed per object by GfxScene) ----
    vec3 L = normalize(obj.lightDir.xyz);
    vec3 radiance = obj.lightColor.rgb;
    float lit = obj.lightDir.w;

    vec3 Lo = vec3(0.0);
    if (lit > 0.5)
    {
        vec3 H = normalize(L + V);
        vec3 F0 = mix(vec3(0.04), albedo, metallic);

        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        vec3 F = FresnelSchlick(VdotH, F0);
        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);

        vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

        Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 color = ambient * albedo                       // ambient term
               + Lo                                     // direct lighting (PBR)
               + emissive;                              // material emission

    // ---- Post-process + gamma (the render target is UNORM) ----
    color = ApplyPostProcess(color);

    outColor = vec4(pow(max(color, vec3(0.0)), vec3(1.0 / 2.2)), 1.0);
}

#version 330 core
layout(location = 0) out vec4 outColor;
uniform sampler2D uPanorama;
uniform samplerCube uEnvironment;
uniform int uPass;
uniform int uFace;
uniform float uSize;
uniform float uRoughness;
const float PI = 3.14159265359;
const uint SAMPLES = 256u;

vec3 FaceDirection(vec2 p) {
    if (uFace == 0) return normalize(vec3(1.0, -p.y, -p.x));
    if (uFace == 1) return normalize(vec3(-1.0, -p.y, p.x));
    if (uFace == 2) return normalize(vec3(p.x, 1.0, p.y));
    if (uFace == 3) return normalize(vec3(p.x, -1.0, -p.y));
    if (uFace == 4) return normalize(vec3(p.x, -p.y, 1.0));
    return normalize(vec3(-p.x, -p.y, -1.0));
}
vec2 Hammersley(uint i) {
    uint b = (i << 16u) | (i >> 16u);
    b = ((b & 0x55555555u) << 1u) | ((b & 0xAAAAAAAAu) >> 1u);
    b = ((b & 0x33333333u) << 2u) | ((b & 0xCCCCCCCCu) >> 2u);
    b = ((b & 0x0F0F0F0Fu) << 4u) | ((b & 0xF0F0F0F0u) >> 4u);
    b = ((b & 0x00FF00FFu) << 8u) | ((b & 0xFF00FF00u) >> 8u);
    return vec2(float(i) / float(SAMPLES), float(b) * 2.3283064365386963e-10);
}
void main() {
    vec3 n = FaceDirection(gl_FragCoord.xy / uSize * 2.0 - 1.0);
    if (uPass == 0) {
        // stb supplies the top scanline first: north (+Y) is v=0.
        vec2 uv = vec2(atan(n.z, n.x) / (2.0 * PI) + 0.5,
                       0.5 - asin(clamp(n.y, -1.0, 1.0)) / PI);
        outColor = vec4(texture(uPanorama, uv).rgb, 1.0);
        return;
    }
    if (uPass == 2 && uRoughness == 0.0) {
        outColor = vec4(textureLod(uEnvironment, n, 0.0).rgb, 1.0);
        return;
    }
    vec3 up = abs(n.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 t = normalize(cross(up, n));
    vec3 b = cross(n, t);
    vec3 sum = vec3(0);
    float weight = 0.0;
    float a2 = pow(uRoughness, 4.0);
    for (uint i = 0u; i < SAMPLES; ++i) {
        vec2 xi = Hammersley(i);
        float phi = 2.0 * PI * xi.x;
        float cosTheta = uPass == 1 ? sqrt(1.0 - xi.y)
            : sqrt((1.0 - xi.y) / (1.0 + (a2 - 1.0) * xi.y));
        float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
        vec3 h = normalize(t * (cos(phi) * sinTheta) + b * (sin(phi) * sinTheta) + n * cosTheta);
        vec3 l = uPass == 1 ? h : normalize(2.0 * dot(n, h) * h - n);
        float NoL = max(dot(n, l), 0.0);
        if (NoL <= 0.0) continue;
        float pdf;
        float w;
        if (uPass == 1) {
            pdf = NoL / PI;
            w = 1.0; // cosine sampling stores irradiance / PI
        } else {
            float d = cosTheta * cosTheta * (a2 - 1.0) + 1.0;
            pdf = a2 / max(4.0 * PI * d * d, 1e-8); // V=N: NoH / VoH cancels
            w = NoL;
        }
        float sampleArea = 1.0 / (float(SAMPLES) * max(pdf, 1e-6));
        float texelArea = 4.0 * PI / (6.0 * 512.0 * 512.0);
        float lod = max(0.0, 0.5 * log2(sampleArea / texelArea));
        sum += textureLod(uEnvironment, l, lod).rgb * w;
        weight += w;
    }
    outColor = vec4(sum / max(weight, 1e-6), 1.0);
}

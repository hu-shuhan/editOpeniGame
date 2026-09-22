#version 420 core

// GPU 光线投射体渲染（阶段 1 验证后端）。
// 在模型局部空间内沿视线做 slab 求交 + 逐采样三线性采样 + 1D LUT 查表，
// front-to-back 合成（预乘 alpha），支持提前终止与首命中深度回写（reversed-z）。

layout(location = 0) in vec2 in_UV;

layout(location = 0) out vec4 out_ScreenColor; // 预乘 alpha：rgb 已乘 a

uniform sampler3D volumeTexture;    // 标量场（R32F）
uniform sampler2D transferFunction;  // RGBA8 一维 LUT（1024x1）

uniform mat4 uInvViewProj;    // (proj * view)^-1
uniform mat4 uInvModel;       // model^-1
uniform mat4 uProjViewModel;  // proj * view * model，用于首命中深度
uniform vec3 uBoxMin;         // 体局部空间包围盒最小角
uniform vec3 uBoxMax;         // 体局部空间包围盒最大角
uniform vec2 uScalarRange;    // (min, max)
uniform float uStepSize;      // 局部空间步长；<=0 时按 uMaxSamples 自动
uniform int uMaxSamples;
uniform vec2 uViewport;

// 返回 (tEnter, tExit)，不相交时 tExit < tEnter
vec2 intersectBox(vec3 origin, vec3 dir, vec3 boxMin, vec3 boxMax) {
    float tEnter = -1e30;
    float tExit = 1e30;
    for (int i = 0; i < 3; ++i) {
        if (abs(dir[i]) < 1e-8) {
            // 光线平行于该轴：若原点不在该 slab 内则不相交
            if (origin[i] < boxMin[i] || origin[i] > boxMax[i]) {
                return vec2(1.0, -1.0);
            }
        } else {
            float invD = 1.0 / dir[i];
            float t0 = (boxMin[i] - origin[i]) * invD;
            float t1 = (boxMax[i] - origin[i]) * invD;
            if (t0 > t1) { float tmp = t0; t0 = t1; t1 = tmp; }
            tEnter = max(tEnter, t0);
            tExit = min(tExit, t1);
            if (tExit < tEnter) { return vec2(1.0, -1.0); }
        }
    }
    return vec2(tEnter, tExit);
}

void main() {
    vec2 ndc = (gl_FragCoord.xy / uViewport) * 2.0 - 1.0;

    // reversed-z：near 平面 z=1.0，far 平面 z=0.0
    vec4 nearW = uInvViewProj * vec4(ndc, 1.0, 1.0);
    nearW /= nearW.w;
    vec4 farW = uInvViewProj * vec4(ndc, 0.0, 1.0);
    farW /= farW.w;

    vec3 nearL = (uInvModel * vec4(nearW.xyz, 1.0)).xyz;
    vec3 farL = (uInvModel * vec4(farW.xyz, 1.0)).xyz;

    vec3 rayDir = normalize(farL - nearL);
    vec3 rayOrigin = nearL;

    vec2 hit = intersectBox(rayOrigin, rayDir, uBoxMin, uBoxMax);
    if (hit.y < hit.x || hit.y < 0.0) {
        discard; // 未命中：保留背景（预乘 alpha=0 不改变帧缓冲）
    }

    float t = max(hit.x, 0.0);
    float tEnd = hit.y;

    float step = uStepSize;
    if (step <= 0.0) {
        step = (tEnd - t) / float(max(uMaxSamples, 1));
    }
    if (step <= 0.0) { step = 1e-4; }

    vec3 boxSize = uBoxMax - uBoxMin;
    float scalarMin = uScalarRange.x;
    float scalarSpan = uScalarRange.y - uScalarRange.x;
    if (abs(scalarSpan) < 1e-12) { scalarSpan = 1.0; }

    // 预乘 alpha 累积
    vec4 accum = vec4(0.0);
    float firstHitT = -1.0;

    for (int i = 0; i < uMaxSamples; ++i) {
        if (t > tEnd) { break; }

        vec3 pos = rayOrigin + rayDir * t;
        vec3 texCoord = (pos - uBoxMin) / boxSize;
        texCoord = clamp(texCoord, vec3(0.0), vec3(1.0));

        float scalar = texture(volumeTexture, texCoord).r;
        float normalized = clamp((scalar - scalarMin) / scalarSpan, 0.0, 1.0);

        vec4 sampleColor = texture(transferFunction, vec2(normalized, 0.5));
        sampleColor.rgb *= sampleColor.a; // 预乘

        accum.rgb += (1.0 - accum.a) * sampleColor.rgb;
        accum.a += (1.0 - accum.a) * sampleColor.a;

        if (firstHitT < 0.0 && sampleColor.a > 0.001) {
            firstHitT = t;
        }

        if (accum.a > 0.995) { break; }
        t += step;
    }

    out_ScreenColor = accum;

    // 首命中深度回写（reversed-z：near=1.0, far=0.0）
    if (firstHitT >= 0.0) {
        vec3 firstHitLocal = rayOrigin + rayDir * firstHitT;
        vec4 clip = uProjViewModel * vec4(firstHitLocal, 1.0);
        gl_FragDepth = clamp(clip.z / clip.w, 0.0, 1.0);
    } else {
        gl_FragDepth = 0.0; // far plane
    }
}

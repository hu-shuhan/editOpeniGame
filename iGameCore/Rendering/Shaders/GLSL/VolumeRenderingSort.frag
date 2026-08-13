#version 420 core

layout(binding = 0, r32ui) uniform readonly uimage2D headPointerImage;
layout(binding = 1, rgba32ui) uniform readonly uimageBuffer listBuffer;

#define MAX_FRAGMENTS 512
#define STEP_BETWEEN_FRAGMENTS 100

uvec4 fragments[MAX_FRAGMENTS];

uniform sampler2D forwardPassColor;

layout(location = 0) in vec2 in_UV;

layout(location = 0) out vec4 out_ScreenColor;

int BuildLocalFragmentList(void) {
    uint current;
    int fragCount = 0;

    current = imageLoad(headPointerImage, ivec2(gl_FragCoord).xy).x;

    while (current!= 0xFFFFFFFF && fragCount < MAX_FRAGMENTS) {
        uvec4 item = imageLoad(listBuffer, int(current));
        fragments[fragCount] = item;
        current = item.x;
        fragCount++;
    }

    return fragCount;
}

void SortFragmentList(int fragCount) {
    int i, j;

    // Insertion Sort
    for (i = 1; i < fragCount; i++) {
        uvec4 currentFragment = fragments[i];
        float currentDist = uintBitsToFloat(currentFragment.z);
        j = i - 1;

        // The fragments are sorted from front to back
        while (j >= 0 && uintBitsToFloat(fragments[j].z) > currentDist) {
            fragments[j + 1] = fragments[j];
            j--;
        }
        fragments[j + 1] = currentFragment;
    }
}

vec3 GetResolveColor() {
    ivec2 texSize = textureSize(forwardPassColor, 0);
    ivec2 texCoord = ivec2(in_UV * vec2(texSize));
    return texelFetch(forwardPassColor, texCoord, 0).rgb;
}

vec3 blend(vec3 currentColor, vec4 newColor) {
    return mix(currentColor, newColor.xyz, newColor.a);
}

vec3 CalculateFinalColor(int fragCount) {
    vec3 background = GetResolveColor();

    if (fragCount == 1) {
        vec4 frag = unpackUnorm4x8(fragments[0].y);
        return mix(background, frag.rgb, frag.a);
    }

    float start_t = uintBitsToFloat(fragments[0].z);
    float end_t = uintBitsToFloat(fragments[fragCount - 1].z);
    float interval_t = end_t - start_t;

    if (interval_t * interval_t < 1e-12f) {
        vec4 frag = unpackUnorm4x8(fragments[0].y);
        return mix(background, frag.rgb, frag.a);
    }

    vec3 finalColor = vec3(0.0f, 0.0f, 0.0f);
    float tau = 0.0f;

    for (int i = 0; i < fragCount - 1; i++) {
        vec3 color1 = unpackUnorm4x8(fragments[i].y).xyz;
        float alpha1 = unpackUnorm4x8(fragments[i].y).a;
        float t1 = (uintBitsToFloat(fragments[i].z) - start_t) / interval_t;

        vec3 color2 = unpackUnorm4x8(fragments[i + 1].y).xyz;
        float alpha2 = unpackUnorm4x8(fragments[i + 1].y).a;
        float t2 = (uintBitsToFloat(fragments[i + 1].z) - start_t) / interval_t;

        float dt = (t2 - t1) / STEP_BETWEEN_FRAGMENTS;
        for (int j = 0; j < STEP_BETWEEN_FRAGMENTS; j++) {
            vec3 color = color1 + j * (color2 - color1)  / float(STEP_BETWEEN_FRAGMENTS);
            float alpha = alpha1 + j * (alpha2 - alpha1) / float(STEP_BETWEEN_FRAGMENTS);
            finalColor += exp(-tau) * alpha * color * dt;
            tau += alpha * dt;
        }
    }

    // remaining transmittance reveals the background color instead of black
    finalColor += exp(-tau) * background;
    return finalColor;
}

void main() {
    int fragCount = BuildLocalFragmentList();

    if (fragCount != 0) {
        SortFragmentList(fragCount);
        out_ScreenColor = vec4(CalculateFinalColor(fragCount), 1.0f);
    } else {
        out_ScreenColor = vec4(GetResolveColor(), 1.0f);
    }
}

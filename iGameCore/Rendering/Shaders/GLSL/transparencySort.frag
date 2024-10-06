#version 420 core

layout(binding = 0, r32ui) uniform readonly uimage2D headPointerImage;
layout(binding = 1, rgba32ui) uniform readonly uimageBuffer listBuffer;

#define MAX_FRAGMENTS 40

uvec4 fragments[MAX_FRAGMENTS];

uniform int numSamples;
uniform sampler2DMS forwardPassColorMS;

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

    for (i = 0; i < fragCount - 1; i++) {
        for (j = i + 1; j < fragCount; j++) {
            float depth_i = uintBitsToFloat(fragments[i].z);
            float depth_j = uintBitsToFloat(fragments[j].z);

            if (depth_i > depth_j) {
                uvec4 temp = fragments[i];
                fragments[i] = fragments[j];
                fragments[j] = temp;
            }
        }
    }
}

vec4 GetResolveColor() {
    ivec2 texSize = textureSize(forwardPassColorMS);
    ivec2 texCoord = ivec2(in_UV * vec2(texSize));

    vec4 color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = 0; i < numSamples; ++i) {
        vec4 sampleColor = texelFetch(forwardPassColorMS, texCoord, i);
        color += sampleColor;
    }
    return color / float(numSamples);
}

vec4 blend(vec4 currentColor, vec4 newColor) {
    return mix(currentColor, newColor, newColor.a);
}

vec4 CalculateFinalColor(int fragCount) {
    vec4 finalColor = GetResolveColor();

    for (int i = 0; i < fragCount; i++) {
        vec4 fragColor = unpackUnorm4x8(fragments[i].y);
        finalColor = blend(finalColor, fragColor);
    }

    return finalColor;
}

void main() {
    int fragCount;

    fragCount = BuildLocalFragmentList();
    SortFragmentList(fragCount);

    out_ScreenColor = CalculateFinalColor(fragCount);
}

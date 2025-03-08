#version 330 core

#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 in_UV;

layout(location = 0) out vec4 out_ScreenColor;

uniform sampler2D screenColorSampler;

//uniform float near;
//uniform float far;
const float near = 0.01;
const float far = 300;

// depth range: 1.0(near plane) -> 0.0(far plane)
// depth \in (near, +infinite)
//float ReverseZLinearizeDepth(float depth) {
//    return near / depth;
//}

// depth \in (0, 1)
float ReverseZLinearizeDepth(float depth) {
    return (near * far) / ((near - far) * depth - near);
}

void main() {
    // color
    {
        vec3 color = vec3(texture(screenColorSampler, in_UV));

        // gamma correct
        // color = pow(color, vec3(1.0f / 2.2f));

        out_ScreenColor = vec4(color, 1.0f);
    }

    // depth
    {
        // ivec2 texSize = textureSize(screenColorSampler, 0);
        // ivec2 texCoord = ivec2(in_UV * vec2(texSize));
        // float depth = texelFetch(screenColorSampler, texCoord, 0).r;
        //
        // // texture(gsampler2D sampler, vec2 P)
        // // float depth = texture(screenColorSampler, in_UV).r;
        //
        // if (depth == 0.0f) {
        //     out_ScreenColor = vec4(0.39215f, 0.58431f, 0.92941f, 1.0f);
        // } else {
        //     float linearDepth = ReverseZLinearizeDepth(depth) / far;
        //     out_ScreenColor = vec4(vec3(linearDepth), 1.0);
        // }
    }

    // depth pyramid
    {
        // float level = 0.0;
        // float depth = textureLod(screenColorSampler, in_UV, level).r;
        // if (depth == 0.0f) {
        //     out_ScreenColor = vec4(0.39215f, 0.58431f, 0.92941f, 1.0f);
        // } else {
        //     float linearDepth = ReverseZLinearizeDepth(depth) / far;
        //     out_ScreenColor = vec4(vec3(linearDepth), 1.0);
        // }
    }
}
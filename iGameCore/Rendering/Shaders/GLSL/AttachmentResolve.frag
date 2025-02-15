#version 330 core

#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 in_UV;

layout(location = 0) out vec4 out_Color;// Output to color attachment 0 (GL_RGBA8 format)
layout(location = 1) out vec4 out_Depth;// Output to color attachment 1 (GL_R32F format)

uniform int numSamples;
uniform sampler2DMS colorTextureMS;
uniform sampler2DMS depthTextureMS;

void main()
{
    // color resolve(from multisamples to single sample)
    {
        ivec2 texSize = textureSize(colorTextureMS);
        ivec2 texCoord = ivec2(in_UV * vec2(texSize));

        vec4 color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        for (int i = 0; i < numSamples; ++i) {
            vec4 sampleColor = texelFetch(colorTextureMS, texCoord, i);
            color += sampleColor;
        }
        out_Color = color / float(numSamples);
    }

    // depth resolve(from multisamples to single sample)
    {
        ivec2 texSize = textureSize(depthTextureMS);
        ivec2 texCoord = ivec2(in_UV * vec2(texSize));

        float minDepth = 1.0;
        for (int i = 0; i < numSamples; ++i) {
            float sampleDepth = texelFetch(depthTextureMS, texCoord, i).r;
            minDepth = min(minDepth, sampleDepth);
        }
        out_Depth  = vec4(minDepth, minDepth, minDepth, 1.0f);
    }
}
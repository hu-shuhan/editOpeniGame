#version 330 core

#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 in_UV;

layout(location = 0) out vec4 out_ScreenColor;

uniform sampler2D fontSampler;
uniform vec3 textColor;

void main()
{
    float alpha = texture(fontSampler, in_UV).r;
    out_ScreenColor = vec4(textColor, alpha);
}
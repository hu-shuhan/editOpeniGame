#version 330 core

#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_UV;

layout (location = 0) out vec2 out_UV;

uniform mat4 proj;

void main()
{
    gl_Position = proj * vec4(in_Position, 1.0);
    out_UV = in_UV;
}
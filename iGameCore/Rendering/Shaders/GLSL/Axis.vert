#version 330 core

#extension GL_ARB_separate_shader_objects : enable

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform bool isDrawFont;

layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Color;
layout(location = 2) in vec3 in_Normal;
layout(location = 3) in vec2 in_UV;

layout(location = 0) out vec3 out_Position;
layout(location = 1) out vec3 out_Color;
layout(location = 2) out vec3 out_Normal;
layout(location = 3) out vec2 out_UV;

void main() {
    gl_Position = proj * view * model * vec4(in_Position, 1.0);

    if (isDrawFont) {
        out_UV = in_UV;
    } else {
        out_Position = vec3(model * vec4(in_Position, 1.0));
        out_Color = in_Color;
        out_Normal = mat3(transpose(inverse(model))) * in_Normal;
    }

}
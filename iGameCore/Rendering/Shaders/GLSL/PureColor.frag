#version 330 core

#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_MCPosition;
layout(location = 1) in vec3 in_VCPosition;
layout(location = 2) in vec4 in_Color;
layout(location = 3) in vec3 in_Normal;
layout(location = 4) in vec2 in_UV;

layout(location = 0) out vec4 out_ScreenColor;

uniform vec3 inputColor = vec3(1.0f, 1.0f, 1.0f);

float cFactor = 0.0f;
float cOffset = 4.0f;

void main() {
    out_ScreenColor = vec4(inputColor, 1.0f);
}

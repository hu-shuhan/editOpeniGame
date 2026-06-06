precision highp float;
attribute vec3 in_Position;
attribute vec3 in_Color;
attribute vec2 in_UV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform bool isDrawFont;

varying vec3 axis_Position;
varying vec3 axis_Color;
varying vec2 axis_UV;

void main() {
    vec4 worldPosition = model * vec4(in_Position, 1.0);
    gl_Position = proj * view * worldPosition;
    axis_Position = worldPosition.xyz;
    axis_Color = in_Color;
    axis_UV = isDrawFont ? in_UV : vec2(0.0);
}

precision highp float;
attribute vec3 in_Position;
attribute vec2 in_UV;

uniform mat4 proj;

varying vec2 out_UV;

void main() {
    gl_Position = proj * vec4(in_Position, 1.0);
    out_UV = in_UV;
}

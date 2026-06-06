precision highp float;
precision highp int;
attribute vec3 in_Position;
attribute vec3 in_Color;
attribute vec3 in_Normal;
attribute vec2 in_UV;

uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uModel;
uniform mat4 uNormal;
uniform int uUseColor;

varying vec3 in_MCPosition;
varying vec3 in_VCPosition;
varying vec3 in_ColorV;
varying vec3 in_NormalV;
varying vec2 in_UVV;

void main() {
    vec4 mc = uModel * vec4(in_Position, 1.0);
    vec4 vc = uView * mc;
    gl_Position = uProj * vc;
    gl_PointSize = 3.0;

    in_MCPosition = mc.xyz;
    in_VCPosition = vc.xyz;
    in_ColorV = (uUseColor == 1) ? in_Color : vec3(0.86, 0.88, 0.92);
    in_NormalV = mat3(uNormal) * in_Normal;
    in_UVV = in_UV;
}

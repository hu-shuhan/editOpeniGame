precision mediump float;
varying vec3 in_MCPosition;
varying vec3 in_VCPosition;
varying vec3 in_ColorV;
varying vec3 in_NormalV;
varying vec2 in_UVV;

void main() {
    gl_FragColor = vec4(in_ColorV, 1.0);
}

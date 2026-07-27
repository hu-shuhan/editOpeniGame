#extension GL_OES_standard_derivatives : enable
precision highp float;

uniform vec3 viewPos;
uniform bool isDrawFont;
uniform sampler2D fontSampler;
uniform vec3 textColor;

varying vec3 axis_Position;
varying vec3 axis_Color;
varying vec2 axis_UV;

void main() {
    if (isDrawFont) {
        float alpha = texture2D(fontSampler, axis_UV).r;
        if (alpha < 0.1) {
            discard;
        }
        gl_FragColor = vec4(textColor, alpha);
        return;
    }

    gl_FragColor = vec4(axis_Color, 1.0);
}

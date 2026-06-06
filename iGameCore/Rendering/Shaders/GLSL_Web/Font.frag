precision mediump float;
varying vec2 out_UV;

uniform sampler2D fontSampler;
uniform vec3 textColor;

void main() {
    float alpha = texture2D(fontSampler, out_UV).r;
    if (alpha <= 0.001) {
        discard;
    }
    gl_FragColor = vec4(textColor, alpha);
}

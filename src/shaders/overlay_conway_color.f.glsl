const float GROW_COLOR = 1;
const float LIVE_COLOR = 0.5;

uniform sampler2D color_texture;
varying vec2      lerp_texcoord;

void main(void) {
    float value = texture2D(color_texture, lerp_texcoord).r;
    if(value == GROW_COLOR) {
        gl_FragColor = vec4(0, 1, 1, 0); // cyan
        return;
    }
    if(value == LIVE_COLOR) {
        gl_FragColor = vec4(0, 0, 1, 0); // blue
        return;
    }
    gl_FragColor = vec4(0, 0, 0, 0); // black
}
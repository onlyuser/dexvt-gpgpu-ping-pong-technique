uniform sampler2D color_texture;
varying vec2      lerp_texcoord;

void main(void) {
    gl_FragColor = texture2D(color_texture, lerp_texcoord);
}
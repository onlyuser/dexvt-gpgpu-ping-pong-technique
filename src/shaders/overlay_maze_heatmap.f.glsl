const float WALL_COLOR   = 0.5;
const float SPRITE_COLOR = 0.25;
const float SEED_COLOR   = 1;

const float threshold[5]    = float[](0.8, 0.6, 0.4, 0.2, 0);
const vec4  maze_heatmap[5] = vec4[](vec4(1, 0, 0, 0),  // red
                                     vec4(1, 1, 0, 0),  // yellow
                                     vec4(0, 1, 0, 0),  // green
                                     vec4(0, 1, 1, 0),  // cyan
                                     vec4(0, 0, 1, 0)); // blue

uniform sampler2D color_texture;
varying vec2      lerp_texcoord;

void main(void) {
    float value = texture2D(color_texture, lerp_texcoord).r;
    if(value == WALL_COLOR) {
        gl_FragColor = vec4(0); // wall (black)
        return;
    }
    if(value == SPRITE_COLOR) {
        gl_FragColor = vec4(1, 0, 1, 0); // sprite (magenta)
        return;
    }
    if(value == SEED_COLOR) {
        gl_FragColor = vec4(1, 1, 1, 0); // seed (white)
        return;
    }
    if(value < SEED_COLOR) {
        value = (value - WALL_COLOR) / (SEED_COLOR - WALL_COLOR); 
        for(int i = 0; i < 5; i++) {
            if(value > SEED_COLOR * threshold[i]) {
                gl_FragColor = mix(maze_heatmap[i], maze_heatmap[i + 1], 1 - (value - threshold[i]) / (threshold[i] - threshold[i + 1]));
                return;
            }
        }
    }
    gl_FragColor = vec4(maze_heatmap[4]); // undefined (blue)
}
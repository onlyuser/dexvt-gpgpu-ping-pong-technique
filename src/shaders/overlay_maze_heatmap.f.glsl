const float WALL_COLOR   = 0.5;
const float SPRITE_COLOR = 0.25;
const float SEED_COLOR   = 1;

const float heatmap_threshold[5] = float[](1, 0.75, 0.5, 0.25, 0);
const vec3  heatmap_colors[5]    = vec3[](vec3(1, 0, 0),  // red
                                          vec3(1, 1, 0),  // yellow
                                          vec3(0, 1, 0),  // green
                                          vec3(0, 1, 1),  // cyan
                                          vec3(0, 0, 1)); // blue

vec3 lerp_heatmap(float value,
                  float min_value,
                  float max_value,
                  bool  min_is_red)
{
    value = clamp(value, min_value, max_value);
    float alpha = (value - min_value) / (max_value - min_value);
    if(min_is_red) {
        alpha = 1 - alpha;
    }
    for(int i = 0; i < 4; i++) {
        if(alpha > heatmap_threshold[i + 1]) {
            float segment_alpha = (alpha - heatmap_threshold[i + 1]) / (heatmap_threshold[i] - heatmap_threshold[i + 1]);
            return mix(heatmap_colors[i + 1], heatmap_colors[i], segment_alpha);
        }
    }
    int heatmap_color_last_index = 4;
    return heatmap_colors[heatmap_color_last_index];
}

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
        gl_FragColor = vec4(lerp_heatmap(value, WALL_COLOR, SEED_COLOR, false), 0);
        return;
    }
    int heatmap_color_last_index = 4;
    gl_FragColor = vec4(heatmap_colors[heatmap_color_last_index], 0); // undefined (blue)
}
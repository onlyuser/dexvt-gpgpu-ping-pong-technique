// Inspired by Sidney Durant's tutorial: https://gamedevelopment.tutsplus.com/tutorials/understanding-goal-based-vector-field-pathfinding--gamedev-9007

const int   MAX_SPRITES    = 100;
const float EMPTY_COLOR    = 0;
const float SPRITE_COLOR   = 0.25;
const float WALL_COLOR     = 0.5;
const float SEED_COLOR     = 1;
const float DECAY_FACTOR   = 0.99;

uniform sampler2D color_texture;
uniform sampler2D color_texture2;
uniform ivec2     viewport_dim;
uniform ivec2     image_res;
uniform ivec2     cursor_pos;
uniform vec2      sprite_pos[MAX_SPRITES];
uniform int       sprite_count;

ivec2 offset[8] = ivec2[](ivec2( 0,  1),  // n
                          ivec2( 1,  1),  // ne
                          ivec2( 1,  0),  // e
                          ivec2( 1, -1),  // se
                          ivec2( 0, -1),  // s
                          ivec2(-1, -1),  // sw
                          ivec2(-1,  0),  // w
                          ivec2(-1,  1)); // nw

float get_pixel(sampler2D texture, ivec2 offset) {
    vec2 pos = vec2(ivec2(gl_FragCoord.xy) + offset) / vec2(image_res - ivec2(1));
    if((pos.x < 0 || pos.x > 1) || (pos.y < 0 || pos.y > 1)) {
        return 0.0;
    }
    return texture2D(texture, pos).r;
}

void main() {
    float merged_color = max(get_pixel(color_texture, ivec2(0)), get_pixel(color_texture2, ivec2(0)));
    if(merged_color == WALL_COLOR) {
        gl_FragColor = vec4(WALL_COLOR); // wall
        return;
    }
    if(merged_color == EMPTY_COLOR) {
        gl_FragColor = vec4(mix(WALL_COLOR, SEED_COLOR, 1 - DECAY_FACTOR)); // empty cell (init within WALL_COLOR..SEED_COLOR range)
        return;
    }
    for(int i = 0; i < sprite_count; i++) {
        if(int(gl_FragCoord.x) == int(sprite_pos[i].x) &&
           int(gl_FragCoord.y) == int(sprite_pos[i].y))
        {
            gl_FragColor = vec4(SPRITE_COLOR); // sprite
            return;
        }
    }
    ivec2 cursor_pos_tex_space = ivec2(int((float(cursor_pos.x) / viewport_dim.x) * image_res.x),
                                       int((float(cursor_pos.y) / viewport_dim.y) * image_res.y));
    if(int(gl_FragCoord.x) == cursor_pos_tex_space.x &&
       int(gl_FragCoord.y) == cursor_pos_tex_space.y)
    {
        gl_FragColor = vec4(SEED_COLOR); // seed
        return;
    }
    float max_value = 0;
    for(int i = 0; i < 8; i++) {
        float current_value = get_pixel(color_texture, offset[i]);
        if(current_value == WALL_COLOR || current_value == SPRITE_COLOR) { // ignore wall cell
            continue;
        }
        max_value = max(max_value, current_value);
    }
    gl_FragColor = vec4(mix(max_value, WALL_COLOR, 1 - DECAY_FACTOR)); // distance field
}
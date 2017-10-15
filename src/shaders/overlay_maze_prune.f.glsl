// Inspired by Sidney Durant's tutorial: https://gamedevelopment.tutsplus.com/tutorials/understanding-goal-based-vector-field-pathfinding--gamedev-9007

const float EMPTY_COLOR = 0;
const float WALL_COLOR  = 0.5;

uniform sampler2D color_texture;
uniform ivec2     image_res;

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
    float current_value = get_pixel(color_texture, ivec2(0));
    if(current_value == EMPTY_COLOR) {
        gl_FragColor = vec4(EMPTY_COLOR); // wall, no change
        return;
    }
    int longest_run       = 0;
    int consecutive_walls = 0;
    int neighbor_walls    = 0;
    for(int i = 0; i < 9; i++) { // loop around one cell
        float current_value = get_pixel(color_texture, offset[int(mod(i, 8))]);
        int   is_wall       = (current_value == WALL_COLOR ? 1 : 0);
        if(i < 8) {
            neighbor_walls += is_wall;
        }
        if(is_wall == 1) {
            consecutive_walls++;
            longest_run = max(longest_run, consecutive_walls);
        } else {
            consecutive_walls = 0;
        }
    }
    if( neighbor_walls <= 1                      ||
       (neighbor_walls == 2 && longest_run == 2) ||
       (neighbor_walls == 3 && longest_run == 3)) // turns into empty cell if less than 3 consecutive neighbor walls
    {
        gl_FragColor = vec4(EMPTY_COLOR);
        return;
    }
    gl_FragColor = vec4(current_value);
}
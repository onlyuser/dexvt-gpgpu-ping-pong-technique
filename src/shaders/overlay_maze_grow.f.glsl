// Inspired by Sidney Durant's tutorial: https://gamedevelopment.tutsplus.com/tutorials/understanding-goal-based-vector-field-pathfinding--gamedev-9007

const float WALL_COLOR = 0.5;

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
    if(current_value == WALL_COLOR) {
        gl_FragColor = vec4(WALL_COLOR); // wall, no change
        return;
    }
    int   transitions = 0;
    float prev_value  = 0;
    int   sum         = 0;
    for(int i = 0; i < 9; i++) { // loop around one cell
        float current_value = get_pixel(color_texture, offset[int(mod(i, 8))]);
        int   is_wall       = (current_value == WALL_COLOR ? 1 : 0);
        if(i < 8) {
            sum += is_wall;
        }
        if(i != 0 && current_value != prev_value) {
            transitions++;
        }
        prev_value = current_value;
    }
    if(sum >= 4 &&       // 3 or more adjacent wall cells means it's either in a corner or on a side; 4 or more means it's even more wall-ish
       transitions == 2) // 2 transitions means adding a wall cell doesn't change topology
    {
        gl_FragColor = vec4(WALL_COLOR);
        return;
    }
    gl_FragColor = vec4(current_value);
}
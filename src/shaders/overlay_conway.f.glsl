// Based on Chris Wellons' tutorial: http://nullprogram.com/blog/2014/06/10/

const float GROW_COLOR = 1;
const float LIVE_COLOR = 0.5;
const float DIE_COLOR  = 0;

uniform sampler2D color_texture;
uniform ivec2     viewport_dim;
uniform ivec2     image_res;
uniform ivec2     cursor_pos;

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
    ivec2 cursor_pos_tex_space = ivec2(int((float(cursor_pos.x) / viewport_dim.x) * image_res.x),
                                       int((float(cursor_pos.y) / viewport_dim.y) * image_res.y));
    if(int(gl_FragCoord.x) == cursor_pos_tex_space.x &&
       int(gl_FragCoord.y) == cursor_pos_tex_space.y)
    {
        gl_FragColor = vec4(GROW_COLOR); // seed
        return;
    }
    int sum = 0;
    for(int i = 0; i < 8; i++) {
        sum += (get_pixel(color_texture, offset[i]) > 0 ? 1 : 0);
    }
    if(sum == 3) {
        gl_FragColor = vec4(GROW_COLOR);
    } else if(sum == 2) {
        vec4 old_color = vec4(get_pixel(color_texture, ivec2(0)));
        if(old_color.r == GROW_COLOR) {
            old_color = vec4(LIVE_COLOR); // add extra transitional color for aesthetic purpose
        }
        gl_FragColor = old_color;
    } else {
        gl_FragColor = vec4(DIE_COLOR);
    }
}
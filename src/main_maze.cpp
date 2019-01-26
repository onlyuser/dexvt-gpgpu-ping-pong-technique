/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Sylvain Beucler
 * Enhanced by: onlyuser
 */

/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* Using the GLUT library for the base windowing setup */
#include <GL/glut.h>
#include <Camera.h>
#include <FrameBuffer.h>
#include <Material.h>
#include <Mesh.h>
#include <PrimitiveFactory.h>
#include <Scene.h>
#include <Texture.h>
#include <Util.h>
#include <sstream> // std::stringstream
#include <iomanip> // std::setprecision
#include <math.h>

#include <cfenv>

#define HI_RES_TEX_DIM (64 - 1)
#define HALF_DIM       static_cast<int>(HI_RES_TEX_DIM * 0.5)
#define SPRITE_COUNT   10
#define EMPTY_COLOR    0.0
#define SPRITE_COLOR   0.25
#define WALL_COLOR     0.5
#define SEED_COLOR     1.0
#define SPRITE_PERIOD  1
#define PRUNE_PERIOD   10
#define GROW_PERIOD    10

#define SPRITE_VELOCITY       1
#define SPRITE_ANGLE_VELOCITY (PI * 0.1)

const char* DEFAULT_CAPTION = "";

int init_screen_width  = 800,
    init_screen_height = 800;
vt::Camera* camera = NULL;
vt::Mesh *mesh = NULL;
vt::Texture *maze_pattern_texture = NULL, // input/output
            *maze_texture         = NULL, // input/output
            *maze_texture2        = NULL; // input/output
vt::Material *write_through_material  = NULL,
             *maze_heatmap_material   = NULL,
             *maze_prune_material     = NULL,
             *maze_grow_material      = NULL,
             *maze_distfield_material = NULL;
vt::FrameBuffer *maze_fb  = NULL, // input/output
                *maze_fb2 = NULL; // input/output

bool left_mouse_down  = false,
     right_mouse_down = false;
glm::vec2 prev_mouse_coord,
          mouse_drag;
float orbit_radius = 8;
bool show_fps      = false,
     do_animation  = true;

enum maze_phases_t {
    MAZE_PHASE_GEN,
    MAZE_PHASE_PRUNE,
    MAZE_PHASE_GROW,
    MAZE_PHASE_DISTFIELD
};
maze_phases_t current_maze_phase     = MAZE_PHASE_GEN;
int           maze_phase_durations[] = {1, 100, 100, -1};
int           tick_count             = 0;

bool skip_prune = true,
     skip_grow  = true;

// generate maze using Prim's algorithm
void gen_maze_pattern(vt::Texture *texture)
{
    texture->set_color_r32f(WALL_COLOR); // clear
    for(int y = 1; y < HI_RES_TEX_DIM; y += 2) {
        for(int x = 1; x < HI_RES_TEX_DIM; x += 2) {
            texture->set_pixel_r32f(glm::ivec2(x, y), EMPTY_COLOR);
        }
    }
    bool visited[HALF_DIM][HALF_DIM];
    std::vector<glm::ivec2> frontier;
    frontier.push_back(glm::ivec2(rand() / (static_cast<float>(RAND_MAX) + 1) * HALF_DIM,
                                  rand() / (static_cast<float>(RAND_MAX) + 1) * HALF_DIM));
    glm::ivec2 offset_4[] = {
        glm::ivec2( 0,  1), // n
        glm::ivec2( 0, -1), // s
        glm::ivec2( 1,  0), // e
        glm::ivec2(-1,  0)  // w
        };
    glm::ivec2 halfdim_min(0), halfdim_max(HALF_DIM - 1);
    while(frontier.size()) {
        int seed_index = rand() / (static_cast<float>(RAND_MAX) + 1) * frontier.size();
        glm::ivec2 seed = frontier[seed_index];
        for(int i = 0; i < 4; i++) {
            glm::ivec2 sample = glm::clamp(seed + offset_4[i], halfdim_min, halfdim_max);
            if(sample != seed && !visited[sample.x][sample.y]) {
                texture->set_pixel_r32f(glm::ivec2(1) + sample * 2 - offset_4[i], EMPTY_COLOR);
                frontier.push_back(sample);
                visited[sample.x][sample.y] = true;
            }
        }
        frontier.erase(frontier.begin() + seed_index);
    }
    //texture->draw_frame();
}

void init_maze()
{
    // initial pattern
#if 1
    // maze pattern
    gen_maze_pattern(maze_texture);
#else
    // closed box
    maze_texture->set_color_r32f(0);
    maze_texture->draw_frame();
#endif
    maze_texture2->set_color_r32f(0);

    // upload to gpu (very slow)
    maze_texture->update();
    maze_texture2->update();
}

void init_prune_maze()
{
    // download from gpu (very slow)
    maze_texture->refresh();
    maze_texture2->refresh();
}

void init_grow_maze()
{
    // download from gpu (very slow)
    maze_texture->refresh();
    maze_texture2->refresh();
}

void init_sprites()
{
    vt::Scene* scene = vt::Scene::instance();

    for(int i = 0; i < SPRITE_COUNT; i++) {
        glm::ivec2 respawn_point;
        do {
            respawn_point = glm::ivec2(rand() / (static_cast<float>(RAND_MAX) + 1) * HALF_DIM * 2,
                                       rand() / (static_cast<float>(RAND_MAX) + 1) * HALF_DIM * 2);
        } while(maze_texture->get_pixel_r32f(respawn_point) == WALL_COLOR);
        scene->set_sprite_pos(i, glm::vec2(respawn_point));
    }
    scene->set_sprite_count(SPRITE_COUNT);
    int sprite_count = scene->get_sprite_count();
    for(int i = 0; i < sprite_count; i++) {
        scene->set_sprite_velocity(i, SPRITE_VELOCITY);
        scene->set_sprite_angle_velocity(i, SPRITE_ANGLE_VELOCITY);
    }
}

void init_distfield_maze()
{
    // initial pattern
    //gen_maze_pattern(maze_pattern_texture); // instead, use result from grow iteration
    maze_texture->set_color_r32f(0);
    maze_texture2->set_color_r32f(0);

    // upload to gpu (very slow)
    maze_pattern_texture->update();
    maze_texture->update();
    maze_texture2->update();

    // for sprites
    init_sprites();
}

int init_resources()
{
    //============
    // scene setup
    //============

    vt::Scene* scene = vt::Scene::instance();

    glm::vec3 origin = glm::vec3();
    camera = new vt::Camera("camera", origin + glm::vec3(0, 0, orbit_radius), origin);
    camera->set_image_res(glm::ivec2(HI_RES_TEX_DIM, HI_RES_TEX_DIM));
    scene->set_camera(camera);

    //=========
    // textures
    //=========

    // input
    maze_pattern_texture = new vt::Texture("maze_pattern",
                                           vt::Texture::RED,
                                           glm::ivec2(HI_RES_TEX_DIM, HI_RES_TEX_DIM),
                                           false); // no lerp (need exact values)

    // input/output
    maze_texture = new vt::Texture("maze",
                                   vt::Texture::RED,
                                   glm::ivec2(HI_RES_TEX_DIM, HI_RES_TEX_DIM),
                                   false); // no lerp (need exact values)
    maze_fb = new vt::FrameBuffer(maze_texture, camera);

    // input/output
    maze_texture2 = new vt::Texture("maze2",
                                    vt::Texture::RED,
                                    glm::ivec2(HI_RES_TEX_DIM, HI_RES_TEX_DIM),
                                    false); // no lerp (need exact values)
    maze_fb2 = new vt::FrameBuffer(maze_texture2, camera);

    //==========
    // materials
    //==========

    // for display
    write_through_material = new vt::Material("write_through",
                                              "src/shaders/overlay_write_through.v.glsl",
                                              "src/shaders/overlay_write_through.f.glsl",
                                              true); // use_overlay
    write_through_material->add_texture(maze_pattern_texture);
    write_through_material->add_texture(maze_texture);
    write_through_material->add_texture(maze_texture2);
    scene->add_material(write_through_material);

    // for maze_heatmap display
    maze_heatmap_material = new vt::Material("maze_heatmap",
                                             "src/shaders/overlay_maze_heatmap.v.glsl",
                                             "src/shaders/overlay_maze_heatmap.f.glsl",
                                             true); // use_overlay
    maze_heatmap_material->add_texture(maze_pattern_texture);
    maze_heatmap_material->add_texture(maze_texture);
    maze_heatmap_material->add_texture(maze_texture2);
    scene->add_material(maze_heatmap_material);

    // for maze pruning
    maze_prune_material = new vt::Material("maze_prune",
                                           "src/shaders/overlay_maze_prune.v.glsl",
                                           "src/shaders/overlay_maze_prune.f.glsl",
                                           true); // use_overlay
    maze_prune_material->add_texture(maze_pattern_texture);
    maze_prune_material->add_texture(maze_texture);
    maze_prune_material->add_texture(maze_texture2);
    scene->add_material(maze_prune_material);

    // for maze growing
    maze_grow_material = new vt::Material("maze_grow",
                                          "src/shaders/overlay_maze_grow.v.glsl",
                                          "src/shaders/overlay_maze_grow.f.glsl",
                                          true); // use_overlay
    maze_grow_material->add_texture(maze_pattern_texture);
    maze_grow_material->add_texture(maze_texture);
    maze_grow_material->add_texture(maze_texture2);
    scene->add_material(maze_grow_material);

    // for distfield rendering
    maze_distfield_material = new vt::Material("maze_distfield",
                                               "src/shaders/overlay_maze_distfield.v.glsl",
                                               "src/shaders/overlay_maze_distfield.f.glsl",
                                               true); // use_overlay
    maze_distfield_material->add_texture(maze_pattern_texture);
    maze_distfield_material->add_texture(maze_texture);
    maze_distfield_material->add_texture(maze_texture2);
    scene->add_material(maze_distfield_material);

    //==============
    // scene setup 2
    //==============

    // for display
    mesh = vt::PrimitiveFactory::create_viewport_quad("overlay");
    mesh->set_material(write_through_material);
    mesh->set_texture_index(mesh->get_material()->get_texture_index_by_name("maze"));
    scene->set_overlay(mesh);

    return 1;
}

int deinit_resources()
{
    return 1;
}

void onIdle()
{
    glutPostRedisplay();
}

void do_maze_prune_iter(vt::Scene*       scene,
                        vt::Texture*     input_texture, // IN
                        vt::FrameBuffer* output_fb)     // OUT
{
    vt::Mesh* mesh = scene->get_overlay();
    vt::Texture* output_texture = output_fb->get_texture();

    static int count = 0;
    if(count == PRUNE_PERIOD) {
        count = 0;

        // enter gpu kernel
        output_fb->bind();
        mesh->set_material(maze_prune_material);
        mesh->set_texture_index(mesh->get_material()->get_texture_index(input_texture));
        scene->render(false, true);
        output_fb->unbind();

        // switch to write-through mode to display final output texture
#if 1
        mesh->set_material(maze_heatmap_material);
#else
        mesh->set_material(write_through_material);
#endif
        mesh->set_texture_index(mesh->get_material()->get_texture_index(output_texture));
        return;
    }
    count++;
}

void do_maze_grow_iter(vt::Scene*       scene,
                       vt::Texture*     input_texture, // IN
                       vt::FrameBuffer* output_fb)     // OUT
{
    vt::Mesh* mesh = scene->get_overlay();
    vt::Texture* output_texture = output_fb->get_texture();

    static int count = 0;
    if(count == GROW_PERIOD) {
        count = 0;

        // enter gpu kernel
        output_fb->bind();
        mesh->set_material(maze_grow_material);
        mesh->set_texture_index(mesh->get_material()->get_texture_index(input_texture));
        scene->render(false, true);
        output_fb->unbind();

        // switch to write-through mode to display final output texture
#if 1
        mesh->set_material(maze_heatmap_material);
#else
        mesh->set_material(write_through_material);
#endif
        mesh->set_texture_index(mesh->get_material()->get_texture_index(output_texture));
        return;
    }
    count++;
}

void do_maze_distfield_iter(vt::Scene*       scene,
                            vt::Texture*     input_texture, // IN
                            vt::FrameBuffer* output_fb)     // OUT
{
    vt::Mesh* mesh = scene->get_overlay();
    vt::Texture* output_texture = output_fb->get_texture();

    // enter gpu kernel
    output_fb->bind();
    mesh->set_material(maze_distfield_material);
    mesh->set_texture_index(mesh->get_material()->get_texture_index(input_texture));
    mesh->set_texture2_index(mesh->get_material()->get_texture_index(maze_pattern_texture));
    scene->render(false, true);
    output_fb->unbind();

    // switch to write-through mode to display final output texture
#if 1
    mesh->set_material(maze_heatmap_material);
#else
    mesh->set_material(write_through_material);
#endif
    mesh->set_texture_index(mesh->get_material()->get_texture_index(output_texture));

    // move sprites along distance field gradient
    static int count = 0;
    if(count == SPRITE_PERIOD) {
        glm::ivec2 offset_8[] = {
            glm::ivec2( 0,  1), // n
            glm::ivec2( 1,  1), // ne
            glm::ivec2( 1,  0), // e
            glm::ivec2( 1, -1), // se
            glm::ivec2( 0, -1), // s
            glm::ivec2(-1, -1), // sw
            glm::ivec2(-1,  0), // w
            glm::ivec2(-1,  1)  // nw
            };
        maze_texture->refresh(); // download from gpu (very slow; unfortunately, we do it on the cpu)
        int sprite_count = scene->get_sprite_count();
        for(int i = 0; i < sprite_count; i++) {
            glm::vec2 pos = scene->get_sprite_pos(i);
            float max_value = std::min(WALL_COLOR, SEED_COLOR);
            float min_value = std::max(WALL_COLOR, SEED_COLOR);
            glm::vec2 max_offset;
            glm::vec2 min_offset;
            for(int j = 0; j < 8; j++) {
                glm::vec2 move_to_point = glm::normalize(glm::vec2(offset_8[j]));
                float value_neighbor = maze_texture->get_pixel_r32f(glm::ivec2(pos + move_to_point));
                if(value_neighbor == WALL_COLOR || value_neighbor == SPRITE_COLOR) {
                    continue;
                }
                if(value_neighbor > max_value) {
                    max_value  = value_neighbor;
                    max_offset = move_to_point;
                }
                if(value_neighbor < min_value) {
                    min_value  = value_neighbor;
                    min_offset = move_to_point;
                }
            }
            if(fabs(max_value - SEED_COLOR) < EPSILON) { // respawn if near target
                glm::ivec2 respawn_point;
                do {
                    respawn_point = glm::ivec2(rand() / (static_cast<float>(RAND_MAX) + 1) * HALF_DIM * 2,
                                               rand() / (static_cast<float>(RAND_MAX) + 1) * HALF_DIM * 2);
                } while(maze_texture->get_pixel_r32f(respawn_point) == WALL_COLOR);
                scene->set_sprite_pos(i, glm::vec2(respawn_point));
                continue;
            }
            if(max_value == min_value) { // equally attractive choices
                glm::vec2 move_to_point;
                do {
                    int seed_index = rand() / (static_cast<float>(RAND_MAX) + 1) * 8; // pick one at random
                    move_to_point = glm::normalize(glm::vec2(offset_8[seed_index]));
                } while(maze_texture->get_pixel_r32f(glm::ivec2(pos + move_to_point)) == WALL_COLOR);
                scene->set_sprite_pos(i, pos + move_to_point);
                continue;
            }
#if 1
            // simple path finding
            scene->set_sprite_pos(i, pos + max_offset);
#else
            // fancy path finding (limit turning speed)
            float target_angle   = -atan2(max_offset.x, max_offset.y) + PI * 0.5;
            float angle          = scene->get_sprite_angle(i);
                  angle          = glm::radians(vt::angle_modulo(glm::degrees(angle)));
            float angle_velocity = scene->get_sprite_angle_velocity(i);
            int   turn_polarity  = (fabs(target_angle - angle) < PI ? 1 : -1);
            if(target_angle > angle) {
                angle += angle_velocity * turn_polarity;
            } else if(target_angle < angle) {
                angle -= angle_velocity * turn_polarity;
            }
            scene->set_sprite_angle(i, angle);
            glm::vec2 new_pos(pos + glm::vec2(cos(angle), sin(angle)) * scene->get_sprite_velocity(i));
            if(maze_texture->get_pixel_r32f(glm::ivec2(new_pos)) == WALL_COLOR) {
                scene->set_sprite_pos(i, pos + max_offset); // to prevent going through walls
                continue;
            }
            scene->set_sprite_pos(i, new_pos);
#endif
        }
        count = 0;
        return;
    }
    count++;
}

void conduct_maze_iter()
{
    switch(current_maze_phase) {
        case MAZE_PHASE_GEN:
            if(tick_count == 0) {
                init_maze();
                tick_count++;
            } else if(maze_phase_durations[current_maze_phase] > 0 && tick_count < maze_phase_durations[current_maze_phase]) {
                tick_count++;
            } else {
                current_maze_phase = MAZE_PHASE_PRUNE;
                tick_count         = 0;
            }
            break;
        case MAZE_PHASE_PRUNE:
            if(tick_count == 0) {
                if(skip_prune) {
                    // download from gpu (very slow)
                    maze_texture->refresh();
                    maze_texture2->refresh();
                    current_maze_phase = MAZE_PHASE_GROW;
                    tick_count         = 0;
                    return;
                }
                init_prune_maze();
                tick_count++;
            } else if(maze_phase_durations[current_maze_phase] > 0 && tick_count < maze_phase_durations[current_maze_phase]) {
                do_maze_prune_iter(vt::Scene::instance(),
                                   maze_fb->get_texture(), // input_texture
                                   maze_fb2);              // output_fb
                std::swap(maze_fb, maze_fb2); // the elusive ping-pong swap
                tick_count++;
            } else {
                current_maze_phase = MAZE_PHASE_GROW;
                tick_count         = 0;
            }
            break;
        case MAZE_PHASE_GROW:
            if(tick_count == 0) {
                if(skip_grow) {
                    // download from gpu (very slow)
                    maze_texture->refresh();
                    maze_texture2->refresh();
                    current_maze_phase = MAZE_PHASE_DISTFIELD;
                    tick_count         = 0;
                    return;
                }
                init_grow_maze();
                tick_count++;
            } else if(maze_phase_durations[current_maze_phase] > 0 && tick_count < maze_phase_durations[current_maze_phase]) {
                do_maze_grow_iter(vt::Scene::instance(),
                                  maze_fb->get_texture(), // input_texture
                                  maze_fb2);              // output_fb
                std::swap(maze_fb, maze_fb2); // the elusive ping-pong swap
                tick_count++;
            } else {
                current_maze_phase = MAZE_PHASE_DISTFIELD;
                tick_count         = 0;
            }
            break;
        case MAZE_PHASE_DISTFIELD:
            if(tick_count == 0) {
                *maze_pattern_texture = *maze_texture;
                init_distfield_maze();
                tick_count++;
            } else if(maze_phase_durations[current_maze_phase] > 0 && tick_count < maze_phase_durations[current_maze_phase]) {
                tick_count++;
            } else {
                do_maze_distfield_iter(vt::Scene::instance(),
                                       maze_fb->get_texture(), // input_texture
                                       maze_fb2);              // output_fb
                std::swap(maze_fb, maze_fb2); // the elusive ping-pong swap
            }
            break;
    }
}

void onTick()
{
    static unsigned int prev_tick = 0;
    static unsigned int frames = 0;
    unsigned int tick = glutGet(GLUT_ELAPSED_TIME);
    unsigned int delta_time = tick - prev_tick;
    static float fps = 0;
    if(delta_time > 1000) {
        fps = 1000.0 * frames / delta_time;
        frames = 0;
        prev_tick = tick;
    }
    if(show_fps && delta_time > 100) {
        std::stringstream ss;
        ss << std::setprecision(2) << std::fixed << fps << " FPS, "
            << "Mouse: {" << mouse_drag.x << ", " << mouse_drag.y << "}";
        //ss << "Width=" << camera->get_width() << ", Width=" << camera->get_height();
        glutSetWindowTitle(ss.str().c_str());
    }
    frames++;
    if(!do_animation) {
        return;
    }
    conduct_maze_iter();
}

void onDisplay()
{
    if(do_animation) {
        onTick();
    }

    vt::Scene::instance()->render(true, true);

    glutSwapBuffers();
}

void onKeyboard(unsigned char key, int x, int y)
{
    switch(key) {
        case 'f': // frame rate
            show_fps = !show_fps;
            if(!show_fps) {
                glutSetWindowTitle(DEFAULT_CAPTION);
            }
            break;
        case 'r': // reset sprites
            init_sprites();
            break;
        case 32: // space
            do_animation = !do_animation;
            break;
        case 27: // escape
            exit(0);
            break;
    }
}

void onSpecial(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_F1:
            // reset maze
            if(tick_count < maze_phase_durations[current_maze_phase]) { // finish previous phase
                break;
            }
            if(maze_fb->get_texture() == maze_texture2) {
                std::swap(maze_fb, maze_fb2);
            }
            current_maze_phase = MAZE_PHASE_GEN;
            tick_count         = 0;
            skip_prune         = true;
            skip_grow          = true;
            break;
        case GLUT_KEY_F2:
            // reset maze + prune
            if(tick_count < maze_phase_durations[current_maze_phase]) { // finish previous phase
                break;
            }
            if(maze_fb->get_texture() == maze_texture2) {
                std::swap(maze_fb, maze_fb2);
            }
            current_maze_phase = MAZE_PHASE_GEN;
            tick_count         = 0;
            skip_prune         = false;
            skip_grow          = true;
            break;
        case GLUT_KEY_F3:
            // reset maze + prune + grow
            if(tick_count < maze_phase_durations[current_maze_phase]) { // finish previous phase
                break;
            }
            if(maze_fb->get_texture() == maze_texture2) {
                std::swap(maze_fb, maze_fb2);
            }
            current_maze_phase = MAZE_PHASE_GEN;
            tick_count         = 0;
            skip_prune         = false;
            skip_grow          = false;
            break;
        case GLUT_KEY_LEFT:
        case GLUT_KEY_RIGHT:
        case GLUT_KEY_UP:
        case GLUT_KEY_DOWN:
            break;
    }
}

void onSpecialUp(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_F1:
        case GLUT_KEY_LEFT:
        case GLUT_KEY_RIGHT:
        case GLUT_KEY_UP:
        case GLUT_KEY_DOWN:
            break;
    }
}

void onMouse(int button, int state, int x, int y)
{
}

void onMotion(int x, int y)
{
}

void onPassiveMotion(int x, int y)
{
    vt::Scene::instance()->set_cursor_pos(glm::ivec2(x, camera->get_height() - y));
}

void onReshape(int width, int height)
{
    camera->resize(0, 0, width, height);
    glViewport(0, 0, width, height);
}

int main(int argc, char* argv[])
{
#if 1
    // https://stackoverflow.com/questions/5393997/stopping-the-debugger-when-a-nan-floating-point-number-is-produced/5394095
    feenableexcept(FE_INVALID | FE_OVERFLOW);
#endif

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(init_screen_width, init_screen_height);
    glutCreateWindow(DEFAULT_CAPTION);

    GLenum glew_status = glewInit();
    if(glew_status != GLEW_OK) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
        return 1;
    }

    if(!GLEW_VERSION_2_0) {
        fprintf(stderr, "Error: your graphic card does not support OpenGL 2.0\n");
        return 1;
    }

    printf("GLSL version %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    if(init_resources()) {
        glutDisplayFunc(onDisplay);
        glutKeyboardFunc(onKeyboard);
        glutSpecialFunc(onSpecial);
        glutSpecialUpFunc(onSpecialUp);
        glutMouseFunc(onMouse);
        glutMotionFunc(onMotion);
        glutPassiveMotionFunc(onPassiveMotion);
        glutReshapeFunc(onReshape);
        glutIdleFunc(onIdle);
        //glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glutMainLoop();
        deinit_resources();
    }

    return 0;
}

/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Sylvain Beucler
 * Enhanced by: Jerry Chen
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
#include <sstream> // std::stringstream
#include <iomanip> // std::setprecision

#define HI_RES_TEX_DIM 128

const char* DEFAULT_CAPTION = "";

int init_screen_width  = 800,
    init_screen_height = 800;
vt::Camera* camera = NULL;
vt::Mesh *mesh = NULL;
vt::Texture *conway_texture  = NULL, // input/output
            *conway_texture2 = NULL; // input/output
vt::Material *write_through_material  = NULL,
             *conway_color_material = NULL,
             *conway_material         = NULL;
vt::FrameBuffer *conway_fb  = NULL, // input/output
                *conway_fb2 = NULL; // input/output

bool left_mouse_down  = false,
     right_mouse_down = false;
glm::vec2 prev_mouse_coord,
          mouse_drag;
float orbit_radius = 8;
bool show_fps      = false,
     do_animation  = true;

void init_conway()
{
    // initial pattern
    conway_texture->set_color_r32f(0);
    conway_texture->draw_x();

    // upload to gpu (very slow)
    conway_texture->update();
    conway_texture2->update();

    // reset cursor
    vt::Scene::instance()->set_cursor_pos(glm::ivec2(0, 0));
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

    // input/output
    conway_texture = new vt::Texture("conway",
                                     vt::Texture::RED,
                                     glm::ivec2(HI_RES_TEX_DIM, HI_RES_TEX_DIM),
                                     false); // no lerp (need exact values)
    conway_fb = new vt::FrameBuffer(conway_texture, camera);

    // input/output
    conway_texture2 = new vt::Texture("conway2",
                                      vt::Texture::RED,
                                      glm::ivec2(HI_RES_TEX_DIM, HI_RES_TEX_DIM),
                                      false); // no lerp (need exact values)
    conway_fb2 = new vt::FrameBuffer(conway_texture2, camera);

    //==========
    // materials
    //==========

    // for display
    write_through_material = new vt::Material("write_through",
                                              "src/shaders/overlay_write_through.v.glsl",
                                              "src/shaders/overlay_write_through.f.glsl",
                                              true); // use_overlay
    write_through_material->add_texture(conway_texture);
    write_through_material->add_texture(conway_texture2);
    scene->add_material(write_through_material);

    // for conway_color display
    conway_color_material = new vt::Material("conway_color",
                                             "src/shaders/overlay_conway_color.v.glsl",
                                             "src/shaders/overlay_conway_color.f.glsl",
                                             true); // use_overlay
    conway_color_material->add_texture(conway_texture);
    conway_color_material->add_texture(conway_texture2);
    scene->add_material(conway_color_material);

    // for conway rendering
    conway_material = new vt::Material("conway",
                                       "src/shaders/overlay_conway.v.glsl",
                                       "src/shaders/overlay_conway.f.glsl",
                                       true); // use_overlay
    conway_material->add_texture(conway_texture);
    conway_material->add_texture(conway_texture2);
    scene->add_material(conway_material);

    //==============
    // scene setup 2
    //==============

    // for display
    mesh = vt::PrimitiveFactory::create_viewport_quad("overlay");
    mesh->set_material(write_through_material);
    mesh->set_texture_index(mesh->get_material()->get_texture_index_by_name("conway2"));
    scene->set_overlay(mesh);

    //===============
    // initial values
    //===============

    // initial pattern
    init_conway();

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

void do_conway_iter(vt::Scene*       scene,
                    vt::Texture*     input_texture, // IN
                    vt::FrameBuffer* output_fb)     // OUT
{
    vt::Mesh* mesh = scene->get_overlay();
    vt::Texture* output_texture = output_fb->get_texture();

    // enter gpu kernel
    output_fb->bind();
    mesh->set_material(conway_material);
    mesh->set_texture_index(mesh->get_material()->get_texture_index(input_texture));
    scene->render(false, true);
    output_fb->unbind();

    // switch to write-through mode to display final output texture
    mesh->set_material(conway_color_material);
    mesh->set_texture_index(mesh->get_material()->get_texture_index(output_texture));
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
    do_conway_iter(vt::Scene::instance(),
                   conway_fb->get_texture(), // input_texture
                   conway_fb2);              // output_fb
    std::swap(conway_fb, conway_fb2); // the elusive ping-pong swap
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
        case 'r': // reset pattern
            if(conway_fb->get_texture() == conway_texture2) {
                std::swap(conway_fb, conway_fb2);
            }
            init_conway();
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

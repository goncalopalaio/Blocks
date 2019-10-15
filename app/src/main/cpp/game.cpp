//
// Created by Gonçalo Palaio on 2019-09-05.
//
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

#include "gp_platform.h"
#include "gp_model.h"
#include "gp_gl.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION

#include "stb_rect_pack.h"
#include "stb_truetype.h"

#define M_MATH_IMPLEMENTATION

#include "m_math.h"
#include "gp_math.h"

#include "game.h"

#include "gp_font_rendering.h"

#include "gp_line_renderer.h"

#define RENDER_MODELS true


// TODO LIST

// [] Make text rendering usable by allowing it to be presented at any position.
// [] Implement mouse picking -- Render 3d object in world below the finger position.
// [] Implement multiple cameras. Free/FlyOver, Fixed
// [] Implement a simple button widget for debugging purposes
// [] Implement a simple button widget for debugging purposes
// [] Implement a simple slider widget for debugging purposes
// [] Implement a simple checkbox widget for debugging purposes
// [] Implement shader hot reloading. Use a default shader as a fallback.
// [] Integrate physics engine (small example just to see how well it works)

typedef struct {
    float3 position;
    float3 direction;
    float3 up;
    float3 look_at;
} Camera;

auto vs_font_source =
        "attribute vec4 vertex_position;\n"
        "attribute vec2 vertex_uvs;\n"
        "uniform mat4 model_matrix;\n"
        "uniform mat4 view_matrix;\n"
        "uniform mat4 projection_matrix;\n"
        "uniform float roll;\n"
        "varying vec2 v_uvs;\n"
        "void main() {\n"
        "  v_uvs = vertex_uvs;"
        "  gl_Position = projection_matrix * view_matrix * model_matrix * vertex_position;\n"
        "}\n";

auto fs_font_source =
        "precision mediump float;\n"
        "uniform sampler2D texture_unit;"
        "varying vec2 v_uvs;\n"
        "void main() {\n"
        //"   gl_FragColor = vec4(v_uvs.y, v_uvs.x, 0.0, 1.0);"
        // "  gl_FragColor = texture2D(texture_unit, v_uvs);\n"
        " float c = texture2D(texture_unit, v_uvs).a;\n"
        "if (c < 0.1) {"
        "   gl_FragColor = vec4(0.1,0.,0.0, 0.0);\n"
        "} else {"
        "   gl_FragColor = vec4(c);\n"
        "}"
        //"  gl_FragColor = vec4(0.0,1.0,0.0,1.0);\n"
        "}\n";

auto vs_textured_source =
        "attribute vec4 vertex_position;\n"
        "attribute vec2 vertex_uvs;\n"
        "uniform mat4 model_matrix;\n"
        "uniform mat4 view_matrix;\n"
        "uniform mat4 projection_matrix;\n"
        "uniform float roll;\n"
        "varying vec2 v_uvs;\n"
        "void main() {\n"
        "  v_uvs = vertex_uvs;"
        "  gl_Position = projection_matrix * view_matrix * model_matrix * vertex_position;\n"
        "}\n";

auto fs_textured_source =
        "precision mediump float;\n"
        "uniform sampler2D texture_unit;"
        "varying vec2 v_uvs;\n"
        "void main() {\n"
        //"   gl_FragColor = vec4(v_uvs.y, v_uvs.x, 0.0, 1.0);"
        "  gl_FragColor = texture2D(texture_unit, v_uvs);\n"
        "}\n";

// Values
float3 ORIGIN = {0, 0, 0};
float3 X_AXIS = {1, 0, 0};
float3 Y_AXIS = {0, 1, 0};
float3 Z_AXIS = {0, 0, 1};

float screen_w = 0;
float screen_h = 0;

float render_tick = 0;

// Input
float touch_x = 0.0f;
float touch_y = 0.0f;
bool touch_is_down = false;
float3 touch_model_trans = {0.0, 0.0, 0.0};
float3 touch_ray_world;

// Fonts
GLuint font_shader_program;
FontData font_data;

// Lines
LineRenderer line_renderer;

// Models
Camera camera;
float view_matrix[] = M_MAT4_IDENTITY();
float projection_matrix[] = M_MAT4_IDENTITY();

float inv_projection_matrix[] = M_MAT4_IDENTITY();
float inv_view_matrix[] = M_MAT4_IDENTITY();

float model_matrix[] = M_MAT4_IDENTITY();

// Temporary matrices
float translation_matrix[] = M_MAT4_IDENTITY();
float rotation_matrix[] = M_MAT4_IDENTITY();
float scale_matrix[] = M_MAT4_IDENTITY();

// Models
SModelData trooper_model;
SModelData plane_model;
SModelData sphere_model;
SModelData duck_model;
SModelData cube_model;

// Textures
GLuint trooper_texture = 0;
GLuint test_texture = 0;
GLuint duck_texture = 0;

/**
 * M_MATH.H extras
 */


/**
 * Game
 */

State init_state_game() {
    log_str("init_state_game");

    struct State state;
    memset(&state, 0, sizeof(state));

    state.w = -1;
    state.h = -1;
    state.valid = false;

    state.total_assets = 1;
    state.assets = (Asset *) malloc(1 * sizeof(Asset));

    return state;
}

void on_resources_loaded_game(Asset *assets, int total_assets) {
    log_str("on_resources_loaded_game");

}

void unload_resources_game(Asset *assets, int total_assets) {
    log_str("unload_resources_game");
}

void
update_camera(Camera *camera, float view_matrix[], float px, float py, float pz, float lx, float ly,
              float lz) {
    m_mat4_identity(view_matrix);

    set_float3(&camera->position, px, py, pz);
    set_float3(&camera->look_at, lx, ly, lz);
    set_float3(&camera->direction, camera->look_at.x - camera->position.x,
               camera->look_at.y - camera->position.y,
               camera->look_at.z - camera->position.z);
    set_float3(&camera->up, 0, 1, 0);
    m_mat4_lookat(view_matrix, &camera->position, &camera->direction, &camera->up);
    m_mat4_inverse(inv_view_matrix, view_matrix);

    log_fmt("Camera - p: %f, %f, %f d: %f, %f, %f", camera->position.x, camera->position.y,
            camera->position.z, camera->direction.x, camera->direction.y, camera->direction.z);


}

GLuint prepare_texture(const char *texture_path, bool flip_on_load) {
    log_fmt("Loading texture %s\n", texture_path);
    int width;
    int height;
    int channels;
    stbi_set_flip_vertically_on_load(flip_on_load);
    unsigned char *pixels = stbi_load(texture_path, &width, &height, &channels, 0);
    assert(pixels != NULL);
    log_fmt("Texture |%s| w: %d h: %d channels: %d is_null?: %d \n", texture_path, width, height,
            channels,
            pixels == NULL);

    //glEnable(GL_TEXTURE_2D);
    GLuint texture = prepare_texture(pixels, width, height, channels);
    stbi_image_free(pixels);
    return texture;
}

void init_game(State *state, int w, int h) {
    log_str("init_game");

    print_gl_string("Version", GL_VERSION);
    print_gl_string("Vendor", GL_VENDOR);
    print_gl_string("Renderer", GL_RENDERER);
    print_gl_string("Extensions", GL_EXTENSIONS);

    state->valid = true;
    state->w = w;
    state->h = h;

    screen_w = w;
    screen_h = h;

    // GL state
    log_fmt("Creating program: Main\n--------------");
    state->main_shader_program = create_program(vs_textured_source, fs_textured_source);
    gl_error("after create_program", __LINE__);

    log_fmt("Creating program: Font\n--------------");
    font_shader_program = create_program(vs_font_source, fs_font_source);
    gl_error("after create_program", __LINE__);

    glViewport(0, 0, w, h);
    gl_error("after viewport", __LINE__);

    // Load models
    char *trooper = read_entire_file("tri_stormt.obj.smodel", 'r');
    trooper_model = parse_smodel_file_as_single_model(trooper);

    char *plane = read_entire_file("plane.obj.smodel", 'r');
    plane_model = parse_smodel_file_as_single_model(plane);

    char *sphere = read_entire_file("sphere.obj.smodel", 'r');
    sphere_model = parse_smodel_file_as_single_model(sphere);

    char *cube = read_entire_file("cube.obj.smodel", 'r');
    cube_model = parse_smodel_file_as_single_model(cube);

    char *duck = read_entire_file("duck.obj.smodel", 'r');
    duck_model = parse_smodel_file_as_single_model(duck);

    //char *cube = read_entire_file("cube.obj.smodel", 'r');
    //char *cube = read_entire_file("plane.obj.smodel", 'r');

    log_fmt("elements_per_vertex: %d vertex_number: %d size: %d has_data: %d",
            trooper_model.elems_per_vertex, trooper_model.vertex_number, trooper_model.size,
            trooper_model.data != nullptr);

    // Scenery positions

    float aspect = w / (float) h;
    m_mat4_perspective(projection_matrix, 13.0, aspect, 0.1, 9999.0);
    m_mat4_inverse(inv_projection_matrix, projection_matrix);

    float dist = 10;
    //update_camera(&camera, view_matrix, dist, 13.000000, dist, 0, 0, 0);
    update_camera(&camera, view_matrix, dist + 4.0, dist, dist, 0, 0, 0);


    log_fmt("Camera position %f %f %f\n", camera.position.x, camera.position.y, camera.position.z);
    log_fmt("Camera direction %f %f %f\n", camera.direction.x, camera.direction.y,
            camera.direction.z);
    log_fmt("Camera up %f %f %f\n", camera.up.x, camera.up.y, camera.up.z);


    // Load images
    trooper_texture = prepare_texture("tri_stormt_ao.png", true);
    test_texture = prepare_texture("texture_map.png", true);
    duck_texture = prepare_texture("duck.png", true);

    font_data = font_init();

    line_renderer_init(&line_renderer, 2724);

    gl_error("end init_game", __LINE__);
}

void transform_touch_screen_to_world(float3 *norm_ray_world, float tx, float ty) {
    // Touch positions must be already screen space(viewport coordinates) - normalized to screen width and in the range of -1,1 where 0,0 is the center of the screen
    // ((2.0f * x) / w) - 1.0f

    // screen space (viewport coordinates)
    float tz = 1.0f;

    // normalised device space
    float3 ray_nds = {tx, ty, tz};
    // clip space
    float4 ray_clip = {ray_nds.x, ray_nds.y, -1.0f, 1.0f};
    // eye space
    float4 ray_eye;

    m_mat4_transform4(&ray_eye, inv_projection_matrix, &ray_clip);
    set_float4(&ray_eye, ray_eye.x, ray_eye.y, -1.0, 0.0);
    // world space
    float4 ray_wor;
    m_mat4_transform4(&ray_wor, inv_view_matrix, &ray_eye);

    set_float3(norm_ray_world, ray_wor.x, ray_wor.y, ray_wor.z);
    f3_ip_normalize(norm_ray_world);
}

void update_touch_input_game(bool is_down, float in_touch_x, float in_touch_y) {
    touch_is_down = is_down;
    if (!is_down) {
        touch_x = 0;
        touch_y = 0;
        set_float3(&touch_ray_world, 0.0,0.0,0.0);
        return;
    }

    touch_x = in_touch_x / screen_w;
    touch_y = in_touch_y / screen_h;

    touch_x = -1.0f + touch_x * 2.0f;
    touch_y = -1.0f + touch_y * 2.0f;

    transform_touch_screen_to_world(&touch_ray_world, touch_x, touch_y);
    log_fmt("touch_x: %f touch_y: %f --- ray_world: %f %f %f", touch_x, touch_y, touch_ray_world.x,
            touch_ray_world.y, touch_ray_world.z);
}

void update_sensor_input_game(float in_yaw, float in_pitch, float in_roll) {
}

void render_model(GLuint shader, GLuint texture, SModelData *model, float model_matrix[],
                  float view_matrix[],
                  float projection_matrix[]) {
    glUseProgram(shader);
    GL_ERR;
    // Render cube
    {
        glActiveTexture(GL_TEXTURE0);
        GL_ERR;
        glBindTexture(GL_TEXTURE_2D, texture);
        GL_ERR;
        GLint texture_unit = glGetAttribLocation(shader,
                                                 "texture_unit");
        GL_ERR;
        glUniform1f(texture_unit, 0.0);
        GL_ERR;

        glUniformMatrix4fv(
                glGetUniformLocation(shader, "model_matrix"),
                1,
                GL_FALSE,
                model_matrix);
        glUniformMatrix4fv(
                glGetUniformLocation(shader, "view_matrix"),
                1,
                GL_FALSE,
                view_matrix);
        glUniformMatrix4fv(
                glGetUniformLocation(shader, "projection_matrix"),
                1,
                GL_FALSE, projection_matrix);

        GLint position = glGetAttribLocation(shader,
                                             "vertex_position");
        GLint uvs = glGetAttribLocation(shader, "vertex_uvs");
        int bytes_per_float = 4;
        int stride = bytes_per_float * model->elems_stride;
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, model->data);
        glEnableVertexAttribArray(uvs);
        glVertexAttribPointer(uvs, 2, GL_FLOAT, GL_FALSE, stride, model->data + 3);
        glDrawArrays(GL_TRIANGLES, 0, model->vertex_number);
    }
    GL_ERR;
    glUseProgram(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void render_game(State *state) {
    render_tick += 0.01f;

    if (touch_is_down) {
        float camera_nudge = 10.01f;
        update_camera(&camera, view_matrix, camera.position.x, camera.position.y,
                      camera.position.z,
                      (touch_x * camera_nudge), (-touch_y * camera_nudge), camera.look_at.z);
    } else {
        /*float camera_nudge = 10.01f;
        float xx = 10 * cos(render_tick);
        float zz = 10 * sin(render_tick);
        update_camera(&camera, view_matrix, xx, camera.position.y, zz,
                      (touch_x * camera_nudge), (-touch_y * camera_nudge), camera.look_at.z);*/
    }

    GL_ERR;
    // Render
    glClearColor(0.2, 0.2, 0.2, 1);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    GL_ERR;

    glUseProgram(state->main_shader_program);
    GL_ERR;

    if (RENDER_MODELS) {
        // Plane
        m_mat4_identity(model_matrix);
        m_mat4_identity(translation_matrix);
        m_mat4_identity(rotation_matrix);
        m_mat4_identity(scale_matrix);

        float3 translation = {0.0, 0.0, 0.0};
        set_float3(&translation, 0, -1.0f, 0.0);
        m_mat4_translation(translation_matrix, &translation);
        m_mat4_rotation_axis(rotation_matrix, &Z_AXIS, M_PI_2);
        m_mat4_mul(model_matrix, translation_matrix, rotation_matrix);
        float3 scale = {0.5, 0.5, 0.5};
        m_mat4_scale(scale_matrix, &scale);
        m_mat4_mul(model_matrix, model_matrix, scale_matrix);

        render_model(state->main_shader_program, trooper_texture, &plane_model, model_matrix,
                     view_matrix, projection_matrix);

        // Plane
        set_float3(&translation, 0, 4.0f, 0.0);
        m_mat4_identity(model_matrix);
        m_mat4_identity(rotation_matrix);
        m_mat4_identity(scale_matrix);
        m_mat4_translation(translation_matrix, &translation);
        m_mat4_rotation_axis(rotation_matrix, &X_AXIS, M_PI + M_PI_2);
        m_mat4_mul(model_matrix, translation_matrix, rotation_matrix);
        set_float3(&scale, 0.5, 0.5, 0.5);
        m_mat4_scale(scale_matrix, &scale);
        m_mat4_mul(model_matrix, model_matrix, scale_matrix);

        render_model(state->main_shader_program, test_texture, &plane_model,
                     model_matrix, view_matrix, projection_matrix);

        // Sphere
        set_float3(&translation, -4.0f, 0.0f, 0.0);
        m_mat4_identity(model_matrix);
        m_mat4_translation(model_matrix, &translation);
        render_model(state->main_shader_program, test_texture, &sphere_model, model_matrix,
                     view_matrix, projection_matrix);

        // Cube
        set_float3(&translation, touch_ray_world.z, touch_ray_world.y, touch_ray_world.z);
        m_mat4_identity(model_matrix);
        m_mat4_translation(model_matrix, &translation);
        render_model(state->main_shader_program, test_texture, &cube_model, model_matrix,
                     view_matrix, projection_matrix);

        // Duck
        set_float3(&translation, 6.0f, 0.0f, 6.0);
        m_mat4_identity(model_matrix);
        m_mat4_translation(model_matrix, &translation);
        render_model(state->main_shader_program, duck_texture, &duck_model, model_matrix,
                     view_matrix, projection_matrix);

        // Trooper
        float offset_space = 1.8f;
        float gx = 1.0f;
        float gy = 1.0f;
        for (float cy = -gy; cy < gy; cy += offset_space) {
            for (float cx = -gx; cx < gx; cx += offset_space) {
                float x = cx;
                float y = touch_model_trans.y;
                float z = cy;
                set_float3(&touch_model_trans, x, y, z);

                m_mat4_identity(model_matrix);
                m_mat4_rotation_axis(model_matrix, &Y_AXIS, render_tick);
                m_mat4_translation(model_matrix, &touch_model_trans);

                render_model(state->main_shader_program, trooper_texture, &trooper_model,
                             model_matrix, view_matrix, projection_matrix);
            }
        }
    }
    glUseProgram(0);

    m_mat4_identity(model_matrix);
    line_renderer_clear_lines(&line_renderer);
    line_renderer_push_point(&line_renderer, 0, 0, 0, 1, 0.5);

    for (int i = 1; i < 10; ++i) {
        line_renderer_push_point(&line_renderer, i, 0, 0, 0, 0.05);
        line_renderer_push_point(&line_renderer, 0, i, 0, 0, 0.05);
        line_renderer_push_point(&line_renderer, 0, 0, i, 0, 0.05);
        line_renderer_push_point(&line_renderer, -i, 0, 0, 0, 0.05);
        line_renderer_push_point(&line_renderer, 0, -i, 0, 0, 0.05);
        line_renderer_push_point(&line_renderer, 0, 0, -i, 0, 0.05);
    }
    line_renderer_push(&line_renderer, 0, 0, 0, 4, 0, 4, 0);

    line_renderer_render(&line_renderer, model_matrix, view_matrix, projection_matrix);

    {
        glDisable(GL_CULL_FACE);
        glUseProgram(font_shader_program);

        float3 translation;
        float3 scale;

        m_mat4_identity(model_matrix);
        m_mat4_identity(rotation_matrix);
        m_mat4_rotation_axis(rotation_matrix, &Y_AXIS, render_tick * 2.0);
        m_mat4_mul(model_matrix, model_matrix, rotation_matrix);
        char buf[500];
        sprintf(buf, "Hi::%f", render_tick);
        font_render(font_data, 0, 0, buf, font_shader_program, model_matrix, view_matrix,
                    projection_matrix);
        glEnable(GL_CULL_FACE);
    }

    glUseProgram(0);

    GL_ERR;
}
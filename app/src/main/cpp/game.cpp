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

#include "stb_truetype.h"

#define M_MATH_IMPLEMENTATION

#include "m_math.h"
#include "gp_math.h"

#include "game.h"

#include "gp_font_rendering.h"

#define RENDER_CUBE true


typedef struct {
    float3 position;
    float3 direction;
    float3 up;
} Camera;

auto vs_font_source =
        "attribute vec4 vertex_position;\n"
        "attribute vec2 vertex_uvs;\n"
        "uniform mat4 model_matrix;\n"
        "uniform mat4 view_matrix;\n"
        "uniform mat4 projection_matrix;\n"
        "varying vec2 v_uvs;\n"
        "varying float color;\n"
        "void main() {\n"
        "  color = vertex_position.z;"
        " vec4 p = vec4(vertex_position.x, vertex_position.y, 0, vertex_position.w);"
        "  v_uvs = vertex_uvs;"
        "  gl_Position = projection_matrix * view_matrix * model_matrix * p;\n"
        "}\n";

auto fs_font_source =
        "precision mediump float;\n"
        "uniform sampler2D texture_unit;"
        "varying vec2 v_uvs;\n"
        "varying float color;\n"
        "void main() {\n"
        //"   gl_FragColor = vec4(color, v_uvs.x, v_uvs.y, 1.0);"
        "  gl_FragColor = 0.5*vec4(color, v_uvs.x, v_uvs.y, 1.0) + 0.5*texture2D(texture_unit, v_uvs);\n"
        //"   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);"
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
const float3 ORIGIN = {0, 0, 0};
const float3 X_AXIS = {1, 0, 0};
const float3 Y_AXIS = {0, 1, 0};
const float3 Z_AXIS = {0, 0, 1};

float screen_w = 0;
float screen_h = 0;

float render_tick = 0;

// Input
float touch_x = 0.0f;
float touch_y = 0.0f;
bool touch_is_down = false;
float3 touch_model_trans = {0.0, 0.0, 0.0};

// Fonts
GLuint font_shader_program;
TextData text_data;
LoadedFont font_data;

// Models
Camera camera;
float view_matrix[] = M_MAT4_IDENTITY();
float projection_matrix[] = M_MAT4_IDENTITY();
float model_matrix[] = M_MAT4_IDENTITY();

SModelData cube_model;

// Textures
GLuint uvs_test_texture = -1;

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

void update_camera(Camera* camera, float view_matrix[], float px, float py, float pz, float lx, float ly, float lz) {
    m_mat4_identity(view_matrix);

    set_float3(&camera->position, px, py, pz);
    set_float3(&camera->direction, lx - camera->position.x, ly - camera->position.y,
               lz - camera->position.z);
    set_float3(&camera->up, 0, 1, 0);
    m_mat4_lookat(view_matrix, &camera->position, &camera->direction, &camera->up);
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
    char *cube = read_entire_file("cube.obj.smodel", 'r');
    cube_model = parse_smodel_file_as_single_model(cube);

    log_fmt("elements_per_vertex: %d vertex_number: %d size: %d has_data: %d",
            cube_model.elems_per_vertex, cube_model.vertex_number, cube_model.size,
            cube_model.data != nullptr);

    // Scenery positions

    float aspect = w / (float) h;
    m_mat4_perspective(projection_matrix, 13.0, aspect, 0.1, 700.0);

    update_camera(&camera, view_matrix, 12,12,12, 0,0,0);


    log_fmt("Camera position %f %f %f\n", camera.position.x, camera.position.y, camera.position.z);
    log_fmt("Camera direction %f %f %f\n", camera.direction.x, camera.direction.y,
            camera.direction.z);
    log_fmt("Camera up %f %f %f\n", camera.up.x, camera.up.y, camera.up.z);


    // Load images
    log_fmt("Loading texture!\n");
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *pixels = stbi_load("tri_stormt_ao.png", &width, &height, &channels, 0);
    assert(pixels != NULL);
    log_fmt("Texture w: %d h: %d channels: %d is_null?: %d \n", width, height, channels,
            pixels == NULL);

    //glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &uvs_test_texture);
    glBindTexture(GL_TEXTURE_2D, uvs_test_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (channels == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     pixels);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);

    // Font loading
    const uint32_t font_size = 11;
    const uint32_t font_atlas_width = 1024;
    const uint32_t font_atlas_height = 1024;
    const uint32_t font_oversample_x = 2;
    const uint32_t font_oversample_y = 2;
    const int font_text_max_length = 16;
    font_init(font_text_max_length, font_size, font_atlas_width, font_atlas_height, font_oversample_x, font_oversample_y, &font_data, &text_data);

    log_loaded_font(&font_data);
    log_text_data(&text_data);

    font_prepare_text(&font_data, &text_data, "AGGGF", text_data.vertex_data, text_data.max_text_length);

    gl_error("end init_game", __LINE__);
}

void update_touch_input_game(bool is_down, float in_touch_x, float in_touch_y) {
    touch_is_down = is_down;
    if (!is_down) {
        touch_x = 0;
        touch_y = 0;
        return;
    }

    touch_x = in_touch_x / screen_w;
    touch_y = in_touch_y / screen_h;

    touch_x = -1.0f + touch_x * 2.0f;
    touch_y = -1.0f + touch_y * 2.0f;
    log_fmt("touch_x: %f touch_y: %f", touch_x, touch_y);
}

void update_sensor_input_game(float in_yaw, float in_pitch, float in_roll) {
}

void render_game(State *state) {
    render_tick += 0.01f;

    if (touch_is_down) {
        float camera_nudge = 8.0f;
        update_camera(&camera, view_matrix, camera.position.x + (touch_x * camera_nudge),camera.position.y + (touch_x * camera_nudge),camera.position.z, 0,0,0);

        set_float3(&camera.direction, 0 - camera.position.x, 0 - camera.position.y,
                   0 - camera.position.z);
        m_mat4_identity(view_matrix);
        m_mat4_lookat(view_matrix, &camera.position, &camera.direction, &camera.up);

    }

    GL_ERR;
    // Render
    glClearColor(0.2, 0.2, 0.2, 1);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    GL_ERR;

    glUseProgram(state->main_shader_program);
    GL_ERR;

    if (RENDER_CUBE) {

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

                // Render cube
                {
                    glActiveTexture(GL_TEXTURE0);
                    GL_ERR;
                    glBindTexture(GL_TEXTURE_2D, uvs_test_texture);
                    GL_ERR;
                    GLint texture_unit = glGetAttribLocation(state->main_shader_program,
                                                             "texture_unit");
                    GL_ERR;
                    glUniform1f(texture_unit, 0.0);
                    GL_ERR;

                    glUniformMatrix4fv(
                            glGetUniformLocation(state->main_shader_program, "model_matrix"),
                            1,
                            GL_FALSE,
                            model_matrix);
                    glUniformMatrix4fv(
                            glGetUniformLocation(state->main_shader_program, "view_matrix"),
                            1,
                            GL_FALSE,
                            view_matrix);
                    glUniformMatrix4fv(
                            glGetUniformLocation(state->main_shader_program, "projection_matrix"),
                            1,
                            GL_FALSE, projection_matrix);

                    GLint position = glGetAttribLocation(state->main_shader_program,
                                                         "vertex_position");
                    GLint uvs = glGetAttribLocation(state->main_shader_program, "vertex_uvs");
                    int bytes_per_float = 4;
                    int stride = bytes_per_float * cube_model.elems_stride;
                    glEnableVertexAttribArray(position);
                    glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, cube_model.data);
                    glEnableVertexAttribArray(uvs);
                    glVertexAttribPointer(uvs, 2, GL_FLOAT, GL_FALSE, stride, cube_model.data + 3);
                    glDrawArrays(GL_TRIANGLES, 0, cube_model.vertex_number);
                }

            }
        }

    }
    glUseProgram(0);

    glUseProgram(font_shader_program);
    m_mat4_identity(model_matrix);
    font_render(font_shader_program, font_data.font_texture, &text_data, model_matrix, view_matrix, projection_matrix);
    glUseProgram(0);

    GL_ERR;
}
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

#define STB_TRUETYPE_IMPLEMENTATION

#include "stb_truetype.h"

#define M_MATH_IMPLEMENTATION

#include "m_math.h"

#include "game.h"

#define SHADER_LOGGING_ON true

#define RENDER_FONT false
#define RENDER_CUBE true


typedef struct {
    float3 position;
    float3 direction;
    float3 up;
} Camera;

auto vs_shader_source =
        "attribute vec4 vertex_position;\n"
        "attribute vec2 vertex_uvs;\n"
        "uniform mat4 model_matrix;\n"
        "uniform mat4 view_matrix;\n"
        "uniform mat4 projection_matrix;\n"
        "uniform float roll;\n"
        "varying float depth;\n"
        "varying vec2 v_uvs;\n"
        "void main() {\n"
        "  v_uvs = vertex_uvs;"
        "  depth = roll + vertex_position.z;\n"
        "  gl_Position = projection_matrix * view_matrix * model_matrix * vertex_position;\n"
        "}\n";

auto fs_shader_source =
        "precision mediump float;\n"
        "varying float depth;\n"
        "uniform sampler2D texture_unit;"
        "varying vec2 v_uvs;\n"
        "void main() {\n"
        //"   gl_FragColor = vec4(v_uvs.y, v_uvs.x, 0.0, 1.0);"
        "  gl_FragColor = texture2D(texture_unit, v_uvs);\n"
        "}\n";

// Models
Camera camera;
float view_matrix[] = M_MAT4_IDENTITY();
float projection_matrix[] = M_MAT4_IDENTITY();
float model_matrix[] = M_MAT4_IDENTITY();

SModelData cube_model;

// Textures
GLuint uvs_test_texture = -1;

static void print_gl_string(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    log_fmt("GL %s = %s\n", name, v);
}

void gl_error(const char *file, int line) {
    bool has_errors = false;
    for (GLint error = glGetError(); error; error = glGetError()) {
        log_fmt("\tGL_ERROR: file:%s:%d -- Hex: 0x%x Dec: %d)\n", file, line, error, error);
        switch (error) {
            case GL_INVALID_ENUM:
                log_fmt("\tINVALID_ENUM");
                break;
            case GL_INVALID_VALUE:
                log_fmt("\tINVALID_VALUE");
                break;
            case GL_INVALID_OPERATION:
                log_fmt("\tINVALID_OPERATION");
                break;
            case GL_OUT_OF_MEMORY:
                log_fmt("\tOUT_OF_MEMORY");
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                log_fmt("\tINVALID_FRAMEBUFFER_OPERATION");
                break;
            default:
                log_fmt("\t__UNEXPECTED_VALUE__)");
        }
        has_errors = true;
    }
    assert(!has_errors);
}

#define GL_ERR gl_error(__FILE__, __LINE__)

void log_shader_info_log(GLuint shader_obj_id) {
    GLint log_length;
    glGetShaderiv(shader_obj_id, GL_INFO_LOG_LENGTH, &log_length);
    GLchar log_buffer[log_length];
    glGetShaderInfoLog(shader_obj_id, log_length, NULL, log_buffer);
    log_fmt("Log:\n %s\n", log_buffer);
}

void log_program_info_log(GLuint program_obj_id) {
    GLint log_length;
    glGetProgramiv(program_obj_id, GL_INFO_LOG_LENGTH, &log_length);
    GLchar log_buffer[log_length];
    glGetProgramInfoLog(program_obj_id, log_length, NULL, log_buffer);

    log_fmt("Log:\n %s\n", log_buffer);
}

GLuint compile_shader(GLenum shader_type, const char *source) {
    assert(source != nullptr);
    GLint compile_status = 0;

    GLuint shader_obj_id = glCreateShader(shader_type);
    assert(shader_obj_id != 0);

    glShaderSource(shader_obj_id, 1, &source, nullptr);
    glCompileShader(shader_obj_id);

    glGetShaderiv(shader_obj_id, GL_COMPILE_STATUS, &compile_status);

    if (SHADER_LOGGING_ON) {
        log_fmt("result for shader compilation: %d\n", shader_type);
        log_shader_info_log(shader_obj_id);
    }

    if (!compile_status) {
        glDeleteShader(shader_obj_id);
    }
    assert(compile_status != 0);

    return shader_obj_id;
}

GLuint create_program(const char *pVertexSource, const char *pFragmentSource) {
    GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, pVertexSource);
    GLuint pixelShader = compile_shader(GL_FRAGMENT_SHADER, pFragmentSource);

    // link program
    GLuint program_obj_id = glCreateProgram();
    assert(program_obj_id != 0);

    glAttachShader(program_obj_id, vertexShader);
    glAttachShader(program_obj_id, pixelShader);
    glLinkProgram(program_obj_id);

    GLint link_status = GL_FALSE;
    glGetProgramiv(program_obj_id, GL_LINK_STATUS, &link_status);

    if (SHADER_LOGGING_ON) {
        log_fmt("result for shader linking:\n");
        log_program_info_log(program_obj_id);
    }

    if (link_status != GL_TRUE) {
        glDeleteProgram(program_obj_id);
    }
    assert(link_status != 0);

    return program_obj_id;
}

/**
 * M_MATH.H extras
 */

void set_float3(float3 *v, float x, float y, float z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

/**
 * Arrays
 */

inline void set_vector3_arr(float *v, float x, float y, float z) {
    *v = x;
    *(v + 1) = y;
    *(v + 2) = z;
}

inline void set_vector2_arr(float *v, float x, float y) {
    *v = x;
    *(v + 1) = y;
}

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

void init_game(State *state, int w, int h) {
    log_str("init_game");

    print_gl_string("Version", GL_VERSION);
    print_gl_string("Vendor", GL_VENDOR);
    print_gl_string("Renderer", GL_RENDERER);
    print_gl_string("Extensions", GL_EXTENSIONS);

    state->valid = true;
    state->w = w;
    state->h = h;

    // GL state
    state->main_shader_program = create_program(vs_shader_source, fs_shader_source);
    gl_error("after create_program", __LINE__);

    glViewport(0, 0, w, h);
    gl_error("after viewport", __LINE__);

    // Load models
    char *cube = read_entire_file("cube.obj.smodel", 'r');
    cube_model = parse_smodel_file(cube);

    log_fmt("elements_per_vertex: %d vertex_number: %d size: %d has_data: %d",
            cube_model.elems_per_vertex, cube_model.vertex_number, cube_model.size,
            cube_model.data != nullptr);

    // Scenery positions

    float aspect = w / (float) h;
    m_mat4_perspective(projection_matrix, 13.0, aspect, 0.1, 100.0);
    m_mat4_identity(view_matrix);

    set_float3(&camera.position, 2.0, 3.0, 2.0);
    set_float3(&camera.direction, 0 - camera.position.x, 0 - camera.position.y,
               0 - camera.position.z);
    set_float3(&camera.up, 0, 1, 0);

    log_fmt("Camera position %f %f %f\n", camera.position.x, camera.position.y, camera.position.z);
    log_fmt("Camera direction %f %f %f\n", camera.direction.x, camera.direction.y,
            camera.direction.z);
    log_fmt("Camera up %f %f %f\n", camera.up.x, camera.up.y, camera.up.z);

    m_mat4_lookat(view_matrix, &camera.position, &camera.direction, &camera.up);


    // Load images
    log_fmt("Loading texture!\n");
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *pixels = stbi_load("texture_map.png", &width, &height, &channels, 0);
    assert(pixels != NULL);
    log_fmt("Texture w: %d h: %d channels: %d is_null?: %d \n", width, height, channels, pixels == NULL);

    //glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &uvs_test_texture);
    glBindTexture(GL_TEXTURE_2D, uvs_test_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (channels == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);

    gl_error("end init_game", __LINE__);
}

void update_input_game(float in_yaw, float in_pitch, float in_roll) {
}

void render_game(State *state) {
    GL_ERR;
    // Render
    glClearColor(0, 0, 1, 1);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    GL_ERR;

    glUseProgram(state->main_shader_program);
    GL_ERR;

    if (RENDER_CUBE) {

        bool draw_texture = true;
        if (draw_texture) {
            glActiveTexture(GL_TEXTURE0);
            GL_ERR;
            glBindTexture(GL_TEXTURE_2D, uvs_test_texture);
            GL_ERR;
            GLint texture_unit = glGetAttribLocation(state->main_shader_program, "texture_unit");
            GL_ERR;
            glUniform1f(texture_unit, 0.0);
            GL_ERR;
        }

        glUniformMatrix4fv(glGetUniformLocation(state->main_shader_program, "model_matrix"), 1,
                           GL_FALSE,
                           model_matrix);
        glUniformMatrix4fv(glGetUniformLocation(state->main_shader_program, "view_matrix"), 1,
                           GL_FALSE,
                           view_matrix);
        glUniformMatrix4fv(glGetUniformLocation(state->main_shader_program, "projection_matrix"), 1,
                           GL_FALSE, projection_matrix);

        GLint position = glGetAttribLocation(state->main_shader_program, "vertex_position");
        GLint uvs = glGetAttribLocation(state->main_shader_program, "vertex_uvs");
        int bytes_per_float = 4;
        int stride = bytes_per_float * cube_model.elems_stride;
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, cube_model.data);
        if (draw_texture) {
            glEnableVertexAttribArray(uvs);
            glVertexAttribPointer(uvs, 2, GL_FLOAT, GL_FALSE, stride, cube_model.data + 3);
        }
        glDrawArrays(GL_TRIANGLES, 0, cube_model.vertex_number);
    }


    glUseProgram(0);
    GL_ERR;
}
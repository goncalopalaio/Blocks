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

#define M_MATH_IMPLEMENTATION

#include "m_math.h"

#include "game.h"

typedef struct {
    float3 position;
    float3 direction;
    float3 up;
} Camera;

auto vs_shader_source =
        "attribute vec4 vPosition;\n"
        "uniform mat4 model_matrix;\n"
        "uniform mat4 view_matrix;\n"
        "uniform mat4 projection_matrix;\n"
        "uniform float roll;\n"
        "varying float depth;\n"
        "void main() {\n"
        "  depth = roll + vPosition.z;\n"
        "  gl_Position = projection_matrix * view_matrix * model_matrix * vPosition;\n"
        "}\n";

auto fs_shader_source =
        "precision mediump float;\n"
        "varying float depth;\n"
        "void main() {\n"
        "  gl_FragColor = vec4(1.0, depth * 2.0, 0.0, 1.0);\n"
        "}\n";

float3 ORIGIN = {0, 0, 0};
float3 X_AXIS = {1, 0, 0};
float3 Y_AXIS = {0, 1, 0};
float3 Z_AXIS = {0, 0, 1};

float view_matrix[] = M_MAT4_IDENTITY();
float projection_matrix[] = M_MAT4_IDENTITY();
float model_matrix[] = M_MAT4_IDENTITY();
SModelData cube_model;

Camera camera;

float delta = 0;
float yaw = 0;
float pitch = 0;
float roll = 0;

static void print_gl_string(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    //logi("GL %s = %s\n", name, v);
    logi(v);
}

static void check_gl_error(const char *op) {
    for (GLint error = glGetError(); error; error
                                                    = glGetError()) {
        // logi("after %s() glError (0x%x)\n", op, error);
        logi("after () glError\n");
    }
}

GLuint loadShader(GLenum shaderType, const char *pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, nullptr);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char *buf = (char *) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, nullptr, buf);
                    // loge("Could not compile shader %d:\n%s\n", shaderType, buf);
                    logi("Could not compile shader %d:\n%s\n");
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint create_program(const char *pVertexSource, const char *pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        check_gl_error("glAttachShader");
        glAttachShader(program, pixelShader);
        check_gl_error("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char *buf = (char *) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    // loge("Could not link program:\n%s\n", buf);
                    logi("Could not link program:\n%s\n");
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
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
 * Game
 */

State init_state_game() {
    logi("init_state_game");

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
    logi("on_resources_loaded_game");

}

void unload_resources_game(Asset *assets, int total_assets) {
    logi("unload_resources_game");

}

void init_game(State *state, int w, int h) {
    logi("init_game");

    print_gl_string("Version", GL_VERSION);
    print_gl_string("Vendor", GL_VENDOR);
    print_gl_string("Renderer", GL_RENDERER);
    print_gl_string("Extensions", GL_EXTENSIONS);

    state->valid = true;
    state->w = w;
    state->h = h;

    // Load models

    char *cube = read_entire_file("cube.obj.smodel", 'r');
    cube_model = parse_smodel_file(cube);

    // @FIXME temporary logging, add support to log other params other than strings
    logi("loaded cube model");
    char* str_buf = (char*) malloc(sizeof(char) * 300);
    sprintf(str_buf, "elements_per_vertex: %d vertex_number: %d size: %d has_data: %d", cube_model.elems_per_vertex, cube_model.vertex_number, cube_model.size, cube_model.data !=
            nullptr);
    logi(str_buf);



    state->main_shader_program = create_program(vs_shader_source, fs_shader_source);
    if (!state->main_shader_program) {
        state->valid = false;
        logi("Could not create program");
        return;
    }

    glViewport(0, 0, w, h);
    check_gl_error("glViewport");

    state->grey = 0.9f;

    float aspect = w / (float) h;
    m_mat4_perspective(projection_matrix, 10.0, aspect, 0.1, 100.0);
    m_mat4_identity(view_matrix);

    set_float3(&camera.position, 2.5, 3.5, 2.5);
    set_float3(&camera.direction, 0 - camera.position.x, 0 - camera.position.y,
               0 - camera.position.z);
    set_float3(&camera.up, 0, 1, 0);

    // logi("Camera position %f %f %f\n", camera.position.x, camera.position.y, camera.position.z);
    // logi("Camera direction %f %f %f\n", camera.direction.x, camera.direction.y, camera.direction.z);
    // logi("Camera up %f %f %f\n", camera.up.x, camera.up.y, camera.up.z);

    m_mat4_lookat(view_matrix, &camera.position, &camera.direction, &camera.up);
}

void update_input_game(float in_yaw, float in_pitch, float in_roll) {
    yaw = in_yaw / 10.0;
    pitch = in_pitch / 10.0;
    roll = in_roll / 10.0;
}

void render_game(State *state) {
    // TODO remove vertices from render loop

    delta += 0.005;
    // Update state
    m_mat4_rotation_axis(model_matrix, &Y_AXIS, cos(delta));
    // Render
    glClearColor(state->grey, state->grey, state->grey, 1);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(state->main_shader_program);
    check_gl_error("glUseProgram_main_shader_program");

    glUniform1f(glGetUniformLocation(state->main_shader_program, "roll"), pitch);
    glUniformMatrix4fv(glGetUniformLocation(state->main_shader_program, "model_matrix"), 1,
                       GL_FALSE,
                       model_matrix);
    glUniformMatrix4fv(glGetUniformLocation(state->main_shader_program, "view_matrix"), 1, GL_FALSE,
                       view_matrix);
    glUniformMatrix4fv(glGetUniformLocation(state->main_shader_program, "projection_matrix"), 1,
                       GL_FALSE, projection_matrix);

    GLint position = glGetAttribLocation(state->main_shader_program, "vPosition");

    if (false) {
        // Triangle
        GLfloat triangle_vertices[] = {1.0f,
                                       1.5f,
                                       -1.5f,
                                       -1.5f,
                                       1.5f,
                                       -1.5f};

        int elementsPerVertex = 2;
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, elementsPerVertex, GL_FLOAT, GL_FALSE, 0, triangle_vertices);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    } else {
        int bytes_per_float = 4;
        int stride = bytes_per_float * cube_model.elems_stride;
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, cube_model.data);
        glDrawArrays(GL_TRIANGLES, 0, cube_model.vertex_number);
    }

    glUseProgram(0);
}
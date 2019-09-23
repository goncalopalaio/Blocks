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
        //"   gl_FragColor = vec4(1.0, depth * 2.0, 0.0, 1.0);"
        "  gl_FragColor = vec4(1.0, depth * 0.5, 0.0, 1.0) + texture2D(texture_unit, v_uvs / 4.0);\n"
        "}\n";


// Font
stbtt_packedchar *font_char_info;
GLuint font_texture = -1;

const uint32_t font_size = 5;
const uint32_t font_atlas_width = 300;
const uint32_t font_atlas_height = 300;
const uint32_t font_oversample_x = 1;
const uint32_t font_oversample_y = 1;
const uint32_t font_first_char = ' ';
const uint32_t font_char_count = '~' - ' ';

const int font_text_max_length = 50;
const int font_elems_per_vertex = 3 + 2;
const int font_elems_per_quad = 4 * font_elems_per_vertex;
float *font_vertex_data;
size_t font_vertex_data_size = sizeof(float) * font_text_max_length * font_elems_per_quad;

// Scenery
Camera camera;
float3 ORIGIN = {0, 0, 0};
float3 X_AXIS = {1, 0, 0};
float3 Y_AXIS = {0, 1, 0};
float3 Z_AXIS = {0, 0, 1};

float view_matrix[] = M_MAT4_IDENTITY();
float projection_matrix[] = M_MAT4_IDENTITY();
float model_matrix[] = M_MAT4_IDENTITY();
SModelData cube_model;

bool is_first_frame = true;

// Input
float delta = 0;
float yaw = 0;
float pitch = 0;
float roll = 0;

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

    // Load models
    char *cube = read_entire_file("cube.obj.smodel", 'r');
    cube_model = parse_smodel_file(cube);

    log_fmt("elements_per_vertex: %d vertex_number: %d size: %d has_data: %d",
            cube_model.elems_per_vertex, cube_model.vertex_number, cube_model.size,
            cube_model.data != nullptr);


    // https://github.com/0xc0dec/demos/blob/master/src/stb-truetype/StbTrueType.cpp
    if (RENDER_FONT) {
        auto *font_buffer = reinterpret_cast<unsigned char *>(read_entire_file("cmunrm.ttf",
                                                                               'r'));
        /* prepare font */
        stbtt_fontinfo info;
        if (!stbtt_InitFont(&info, font_buffer, 0)) {
            log_str("init font failed");
        } else {
            log_str("font loaded");
        }

        auto *atlas_data = (uint8_t *) malloc(
                sizeof(uint8_t) * font_atlas_width * font_atlas_height);
        font_char_info = (stbtt_packedchar *) malloc(sizeof(stbtt_packedchar) * font_char_count);

        stbtt_pack_context context;
        if (!stbtt_PackBegin(&context, atlas_data, font_atlas_width, font_atlas_height, 0, 1,
                             nullptr)) {
            log_str("failed to pack begin font");
            assert(0);
            return;
        }

        stbtt_PackSetOversampling(&context, font_oversample_x, font_oversample_y);
        if (!stbtt_PackFontRange(&context, font_buffer, 0, font_size, font_first_char,
                                 font_char_count, font_char_info)) {
            log_str("failed to pack font");
            assert(0);
            return;
        }

        stbtt_PackEnd(&context);

        glGenTextures(1, &font_texture);
        glBindTexture(GL_TEXTURE_2D, font_texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, font_atlas_width, font_atlas_height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, atlas_data);
        gl_error("game", __LINE__);
        glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
        glGenerateMipmap(GL_TEXTURE_2D);

        free(atlas_data);
        free(font_char_info);
        free(font_buffer);
    }

    // Font loaded, allocate space to render quads

    font_vertex_data = (float *) malloc(font_vertex_data_size);
    memset(font_vertex_data, 0, font_vertex_data_size);

    state->main_shader_program = create_program(vs_shader_source, fs_shader_source);
    if (!state->main_shader_program) {
        state->valid = false;
        log_str("Could not create program");
        return;
    }
    gl_error("after create_program", __LINE__);

    glViewport(0, 0, w, h);

    state->grey = 0.9f;

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


    gl_error("end init_game", __LINE__);
}

void update_input_game(float in_yaw, float in_pitch, float in_roll) {
    yaw = in_yaw / 10.0;
    pitch = in_pitch / 10.0;
    roll = in_roll / 10.0;
}

void fill_glyph_info(const char *text, float *buffer, int font_max_length) {
    // @TODO move allocations outside of render loop
    size_t len = strlen(text);
    assert(font_max_length > len);

    // allocate space for 4 vertices with 2 uv coordinates

    float offset_x = 0;
    float offset_y = 0;

    stbtt_aligned_quad quad;

    int data_idx = 0;

    for (int i = 0; i < len; i++) {
        char character = text[i];
        stbtt_GetPackedQuad(font_char_info,
                            font_atlas_width,
                            font_atlas_height,
                            character - font_first_char,
                            &offset_x,
                            &offset_y,
                            &quad,
                            1);
        auto xmin = quad.x0;
        auto xmax = quad.x1;
        auto ymin = -quad.y1;
        auto ymax = -quad.y0;
        log_fmt("xmin %f xmax %f ymin %f ymax %f", xmin, xmax, ymin, ymax);

        set_vector3_arr(&buffer[data_idx], xmin, ymin, 0);
        data_idx += 3;
        set_vector2_arr(&buffer[data_idx], quad.s0, quad.t1);
        data_idx += 2;

        set_vector3_arr(&buffer[data_idx], xmin, ymax, 0);
        data_idx += 3;
        set_vector2_arr(&buffer[data_idx], quad.s0, quad.t0);
        data_idx += 2;

        set_vector3_arr(&buffer[data_idx], xmax, ymax, 0);
        data_idx += 3;
        set_vector2_arr(&buffer[data_idx], quad.s1, quad.t0);
        data_idx += 2;

        set_vector3_arr(&buffer[data_idx], xmax, ymin, 0);
        data_idx += 3;
        set_vector2_arr(&buffer[data_idx], quad.s1, quad.t1);
        data_idx += 2;
    }
}

void render_game(State *state) {
    // TODO remove vertices from render loop
    GL_ERR;

    delta += 0.01;
    // Update state
    m_mat4_rotation_axis(model_matrix, &Z_AXIS, cos(delta));
    // Render
    glClearColor(state->grey, state->grey, state->grey, 1);

    glClear(GL_DEPTH_BUFFER_BIT);
    gl_error("game", __LINE__);

    glUseProgram(state->main_shader_program);
    gl_error("game", __LINE__);

    glUniform1f(glGetUniformLocation(state->main_shader_program, "roll"), pitch);
    glUniform1f(glGetUniformLocation(state->main_shader_program, "texture_unit"), 0);
    glUniformMatrix4fv(glGetUniformLocation(state->main_shader_program, "model_matrix"), 1,
                       GL_FALSE,
                       model_matrix);
    glUniformMatrix4fv(glGetUniformLocation(state->main_shader_program, "view_matrix"), 1, GL_FALSE,
                       view_matrix);
    glUniformMatrix4fv(glGetUniformLocation(state->main_shader_program, "projection_matrix"), 1,
                       GL_FALSE, projection_matrix);

    GLint position = glGetAttribLocation(state->main_shader_program, "vertex_position");
    GLint uvs = glGetAttribLocation(state->main_shader_program, "vertex_uvs");


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
        glVertexAttribPointer(position, elementsPerVertex, GL_FLOAT, GL_FALSE, 0,
                              triangle_vertices);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    } else {

        bool render_cube = false;
        if (render_cube) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE, font_texture);

            int bytes_per_float = 4;
            int stride = bytes_per_float * cube_model.elems_stride;
            glEnableVertexAttribArray(position);
            glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, cube_model.data);
            glEnableVertexAttribArray(uvs);
            glVertexAttribPointer(uvs, 2, GL_FLOAT, GL_FALSE, stride, cube_model.data + 3);
            glDrawArrays(GL_TRIANGLES, 0, cube_model.vertex_number);
        }

    }

    if (RENDER_FONT) {
        is_first_frame = false;
        size_t text_length = 0;
        if (is_first_frame) {
            is_first_frame = false;

            const char *text = "Hell";
            fill_glyph_info(text, font_vertex_data, font_text_max_length);

            text_length = strlen(text);
        }

        // Render text in quads
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE, font_texture);

        int bytes_per_float = 4;
        int stride = bytes_per_float * (3 + 2);
        gl_error("Before vertex_arrays", __LINE__);
        glEnableVertexAttribArray(position);
        GL_ERR;
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, font_vertex_data);
        GL_ERR;
        glEnableVertexAttribArray(uvs);
        GL_ERR;
        glVertexAttribPointer(uvs, 2, GL_FLOAT, GL_FALSE, stride, font_vertex_data + 3);
        GL_ERR;
        glDrawArrays(GL_TRIANGLES, 0, text_length * 6);
        GL_ERR;
    }


    glUseProgram(0);
}
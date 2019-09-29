//
// Created by Gonçalo Palaio on 2019-09-28.
//

#ifndef BLOCKS_GP_GL_H
#define BLOCKS_GP_GL_H

#include <cassert>

#define SHADER_LOGGING_ON true

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
    log_buffer[0] = '\0';

    glGetShaderInfoLog(shader_obj_id, log_length, NULL, log_buffer);
    log_fmt("Log:\n %s\n", log_buffer);
}

void log_program_info_log(GLuint program_obj_id) {
    GLint log_length;
    glGetProgramiv(program_obj_id, GL_INFO_LOG_LENGTH, &log_length);
    GLchar log_buffer[log_length];
    log_buffer[0] = '\0';

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
    log_fmt("compiling GL_VERTEX_SHADER");
    GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, pVertexSource);
    log_fmt("compiling GL_FRAGMENT_SHADER");
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

#endif //BLOCKS_GP_GL_H

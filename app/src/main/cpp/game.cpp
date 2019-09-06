//
// Created by Gonçalo Palaio on 2019-09-05.
//

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include "game.h"

// Example taken from https://github.com/googlesamples/android-ndk/blob/master/hello-gl2/app/src/main/cpp/gl_code.cpp

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static void print_gl_string(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void check_gl_error(const char *op) {
    for (GLint error = glGetError(); error; error
                                                    = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

auto vs_shader_source =
        "attribute vec4 vPosition;\n"
        "void main() {\n"
        "  gl_Position = vPosition;\n"
        "}\n";

auto fs_shader_source =
        "precision mediump float;\n"
        "void main() {\n"
        "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
        "}\n";

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
                    LOGE("Could not compile shader %d:\n%s\n",
                         shaderType, buf);
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
                    LOGE("Could not link program:\n%s\n", buf);
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
 * Game
 */

State init_game(int w, int h) {
    LOGI("init_game(%d, %d)", w, h);
    print_gl_string("Version", GL_VERSION);
    print_gl_string("Vendor", GL_VENDOR);
    print_gl_string("Renderer", GL_RENDERER);
    print_gl_string("Extensions", GL_EXTENSIONS);

    struct State state;
    memset(&state, 0, sizeof(state));
    state.valid = true;
    state.w = w;
    state.h = h;

    state.main_shader_program = create_program(vs_shader_source, fs_shader_source);
    if (!state.main_shader_program) {
        state.valid = false;
        LOGE("Could not create program");
        return state;
    }

    glViewport(0, 0, w, h);
    check_gl_error("glViewport");

    state.grey = 0.9f;

    return state;
}

void render_game(State *state) {
    // TODO remove vertices from render loop
    GLfloat triangle_vertices[] = {0.0f,
                                   0.5f,
                                   -0.5f,
                                   -0.5f,
                                   0.5f,
                                   -0.5f};
    glClearColor(state->grey, state->grey, state->grey, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    check_gl_error("glClear");

    glUseProgram(state->main_shader_program);
    check_gl_error("glUseProgram_main_shader_program");


    GLint position = glGetAttribLocation(state->main_shader_program, "vPosition");
    int elementsPerVertex = 2;
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, elementsPerVertex, GL_FLOAT, GL_FALSE, 0, triangle_vertices);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    //glUseProgram(0);
    //check_gl_error("glUseProgram_0");
}
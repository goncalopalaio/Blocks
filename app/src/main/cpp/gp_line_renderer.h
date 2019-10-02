//
// Created by Gonçalo Palaio on 2019-10-02.
//

#ifndef BLOCKS_GP_LINE_RENDERER_H
#define BLOCKS_GP_LINE_RENDERER_H

#define GP_LINE_RENDERER_POS_ELEMS 3
#define GP_LINE_RENDERER_COLOR_ELEMS 1

typedef struct {
    int current_lines;
    int max_lines;
    int elements_per_vertex;
    float *vertex_data;
    float *push_ptr;
    GLuint shader;
} LineRenderer;

void line_renderer_init(LineRenderer *renderer, int max_lines) {
    auto vs_source =
            "attribute vec4 vertex_position;\n"
            "attribute float vertex_color_index;\n"
            "uniform mat4 model_matrix;\n"
            "uniform mat4 view_matrix;\n"
            "uniform mat4 projection_matrix;\n"
            "varying float color;"
            "void main() {\n"
            "  color = vertex_color_index;"
            "  gl_Position = projection_matrix * view_matrix * model_matrix * vertex_position;\n"
            "}\n";

    auto fs_source =
            "precision mediump float;\n"
            "varying float color;"
            "void main() {\n"
            "   vec4 colors[5];\n"
            "   colors[0] = vec4(1.0,0.0,0.0, 1.0);\n"
            "   colors[1] = vec4(0.0,1.0,0.0, 1.0);\n"
            "   colors[2] = vec4(0.0,0.0,1.0, 1.0);\n"
            "   colors[3] = vec4(1.0,1.0,0.0, 1.0);\n"
            "   colors[4] = vec4(1.0,0.0,1.0, 1.0);"
            "   gl_FragColor = colors[int(color)];"
            "}\n";

    renderer->shader = create_program(vs_source, fs_source);

    renderer->current_lines = 0;
    renderer->max_lines = max_lines;
    renderer->elements_per_vertex = GP_LINE_RENDERER_POS_ELEMS + GP_LINE_RENDERER_COLOR_ELEMS;
    renderer->vertex_data = (float *) malloc(
            sizeof(float) * renderer->elements_per_vertex * renderer->max_lines * 2);
}

inline void line_renderer_clear_lines(LineRenderer *renderer) {
    renderer->current_lines = 0;
    memset(renderer->vertex_data, 0,
           sizeof(float) * renderer->elements_per_vertex * renderer->max_lines * 2);
    renderer->push_ptr = renderer->vertex_data;
}

inline void
line_renderer_push(LineRenderer *renderer, float ax, float ay, float az, float bx, float by,
                   float bz, int color_index) {
    renderer->current_lines += 1;
    assert(renderer->current_lines < renderer->max_lines);
    renderer->push_ptr = push_v3_arr(renderer->push_ptr, ax, ay, az);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
}

inline void
line_renderer_push_point(LineRenderer *renderer, float px, float py, float pz, int color_index,
                         float side) {
    renderer->current_lines += 6;
    if (renderer->current_lines >= renderer->max_lines) {
        log_fmt("line_renderer_push_point: curr: %d max: %d", renderer->current_lines,
                renderer->max_lines);
    }
    assert(renderer->current_lines < renderer->max_lines);

    renderer->push_ptr = push_v3_arr(renderer->push_ptr, px + side, py, pz);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
    renderer->push_ptr = push_v3_arr(renderer->push_ptr, px, py, pz);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
    renderer->push_ptr = push_v3_arr(renderer->push_ptr, px - side, py, pz);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
    renderer->push_ptr = push_v3_arr(renderer->push_ptr, px, py, pz);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
    renderer->push_ptr = push_v3_arr(renderer->push_ptr, px, py + side, pz);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
    renderer->push_ptr = push_v3_arr(renderer->push_ptr, px, py, pz);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
    renderer->push_ptr = push_v3_arr(renderer->push_ptr, px, py - side, pz);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
    renderer->push_ptr = push_v3_arr(renderer->push_ptr, px, py, pz);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
    renderer->push_ptr = push_v3_arr(renderer->push_ptr, px, py, pz + side);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
    renderer->push_ptr = push_v3_arr(renderer->push_ptr, px, py, pz);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
    renderer->push_ptr = push_v3_arr(renderer->push_ptr, px, py, pz - side);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
    renderer->push_ptr = push_v3_arr(renderer->push_ptr, px, py, pz);
    renderer->push_ptr = push_v1_arr(renderer->push_ptr, color_index);
}

/**
 *
 *
 * fun buildPointLines(colorIndex: Int, position: Vec3, side: Float = 0.05f): ArrayList<Line> {
    val pointInSpace = ArrayList<Line>()

    pointInSpace.add(Line(position.x,position.y ,position.z, position.x, position.y,position.z, colorIndex))

    pointInSpace.add(Line(position.x + side,position.y ,position.z, position.x, position.y,position.z, colorIndex))
    pointInSpace.add(Line(position.x - side,position.y ,position.z, position.x, position.y,position.z, colorIndex))

    pointInSpace.add(Line(position.x ,position.y + side,position.z, position.x, position.y,position.z, colorIndex))
    pointInSpace.add(Line(position.x ,position.y - side,position.z, position.x, position.y,position.z, colorIndex))

    pointInSpace.add(Line(position.x ,position.y, position.z + side, position.x, position.y,position.z, colorIndex))
    pointInSpace.add(Line(position.x ,position.y, position.z - side, position.x, position.y,position.z, colorIndex))

    return pointInSpace
}
 */

inline void line_renderer_render(LineRenderer *renderer, float model_matrix[], float view_matrix[],
                                 float projection_matrix[]) {
    GL_ERR;
    GLuint shader = renderer->shader;
    glUseProgram(shader);

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

    GLint position = glGetAttribLocation(shader, "vertex_position");
    GLint color = glGetAttribLocation(shader, "vertex_color_index");
    int bytes_per_float = 4;
    int stride = bytes_per_float * renderer->elements_per_vertex;
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, GP_LINE_RENDERER_POS_ELEMS, GL_FLOAT, GL_FALSE, stride,
                          renderer->vertex_data);
    glEnableVertexAttribArray(color);
    //GL_ERR;
    glVertexAttribPointer(color, GP_LINE_RENDERER_COLOR_ELEMS, GL_FLOAT, GL_FALSE, stride,
                          renderer->vertex_data + GP_LINE_RENDERER_POS_ELEMS * 2);
    glDrawArrays(GL_LINES, 0, renderer->current_lines * 2);
    glUseProgram(0);
}

#endif //BLOCKS_GP_LINE_RENDERER_H

//
// Created by Gonçalo Palaio on 2019-09-28.
//

#ifndef BLOCKS_GP_FONT_RENDERING_H
#define BLOCKS_GP_FONT_RENDERING_H

#include <cstdio>
/*

typedef struct {
    float *vertex_data;
    size_t vertex_data_size;

    uint32_t cur_text_length;
    uint32_t max_text_length;

    uint32_t elems_per_vertex;
} TextData;

typedef struct {
    GLuint font_texture;
    stbtt_packedchar *font_char_info;

    // @todo review types throughout the project
    uint32_t font_size;
    int font_ascent;
    uint32_t font_first_char;
    uint32_t font_char_count;
    uint32_t font_atlas_width;
    uint32_t font_atlas_height;
    uint32_t font_oversample_x;
    uint32_t font_oversample_y;
} LoadedFont;

void log_loaded_font(LoadedFont *f) {
    log_fmt("LoadedFont - font_texture: %d", f->font_texture);
    log_fmt("LoadedFont - *font_char_info: NotNull? %d", (f->font_char_info != NULL));
    log_fmt("LoadedFont - font_size: %d", f->font_size);
    log_fmt("LoadedFont - font_ascent: %d", f->font_ascent);
    log_fmt("LoadedFont - font_first_char: %d", f->font_first_char);
    log_fmt("LoadedFont - font_char_count: %d", f->font_char_count);
    log_fmt("LoadedFont - font_atlas_width: %d", f->font_atlas_width);
    log_fmt("LoadedFont - font_atlas_height: %d", f->font_atlas_height);
    log_fmt("LoadedFont - font_oversample_x: %d", f->font_oversample_x);
    log_fmt("LoadedFont - font_oversample_y: %d", f->font_oversample_y);
}

void log_text_data(TextData *d) {
    log_fmt("TextData - *vertex_data: NotNull? %d", (d->vertex_data != NULL));
    log_fmt("TextData - vertex_data_size: %d", d->vertex_data_size);
    log_fmt("TextData - cur_text_length: %d", d->cur_text_length);
    log_fmt("TextData - max_text_length: %d", d->max_text_length);
    log_fmt("TextData - elems_per_vertex: %d", d->elems_per_vertex);
}
*/
typedef struct {
    GLuint texture;
    float *vertex_data;
    size_t vertex_data_size;

    int max_text_length;

    stbtt_bakedchar *font_char_data;
    int font_bitmap_width;
    int font_bitmap_height;
    int font_first_char;
} FontData;

GLuint upload_new_texture(int width, int height, int channels, unsigned char* pixels) {
    GLuint tex;

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (channels == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    } else if(channels == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}

FontData font_init() {
    FontData result;
    result.texture = 0;
    result.vertex_data = NULL;
    result.vertex_data_size = 0;
    result.max_text_length = 0;

    // Prepare font
    auto *font_file = reinterpret_cast<unsigned char *>(read_entire_file("cmunrm.ttf",
                                                                           'r'));

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, font_file, 0)) {
        log_str("init font failed");
        assert(0);
        return result;
    } else {
        log_str("font loaded");
    }
    const uint32_t font_size = 100;
    const char font_first_char = ' ';
    const int font_char_count = '~' - ' ';
    const int bitmap_width = 512;
    const int bitmap_height = 512;

    log_fmt("font_init - font_first_char: %d", font_first_char);
    log_fmt("font_init - font_char_count: %d", font_char_count);

    auto *bitmap = (uint8_t *) malloc(
            sizeof(uint8_t) * bitmap_width * bitmap_height);
    auto font_char_data = (stbtt_bakedchar *) malloc(
            sizeof(stbtt_bakedchar) * font_char_count);

    stbtt_BakeFontBitmap(font_file,0, font_size, bitmap, bitmap_width, bitmap_height, font_first_char, font_char_count, font_char_data);

    GLuint font_texture = upload_new_texture(bitmap_width, bitmap_height, 1, bitmap);

    // Cleanup
    free(bitmap);
    free(font_file);

    {
        result.texture = font_texture;
        result.max_text_length = 10;
        result.vertex_data_size = sizeof(float) * result.max_text_length * 6 * 5;

        result.font_char_data = font_char_data;
        result.font_bitmap_width = bitmap_width;
        result.font_bitmap_height = bitmap_height;
        result.font_first_char = font_first_char;

        result.vertex_data = (float *) malloc(result.vertex_data_size);
        memset(result.vertex_data, 0, result.vertex_data_size);
    }
    return result;
}

void font_render(FontData d, float initial_x, float initial_y, const char *text, int shader, float model_matrix[], float view_matrix[], float projection_matrix[]) {
    memset(d.vertex_data, 0, d.vertex_data_size);
    int len = M_MIN(d.max_text_length, strlen(text));
    //log_fmt("rendering %d characters", len);
    float x = initial_x;
    float y = initial_y;
    stbtt_aligned_quad q;
    float* buf = d.vertex_data;
    for (int i = 0; i < len; i++) {
        char character = text[i];
        int index_to_char = character - d.font_first_char;
        log_fmt("rendering %c x: %f y: %f", character, x, y);

        stbtt_GetBakedQuad(d.font_char_data, d.font_bitmap_width, d.font_bitmap_height, index_to_char, &x,&y, &q, 1);
        {
            // invert the y coordinates since the texture is up side down.
            q.y0 = -q.y0;
            q.y1 = -q.y1;

            buf = push_textured_quad_scaled_arr(buf, q.x0, q.y0, q.x1, q.y1, q.s0, q.t0, q.s1, q.t1, 1.1, 1.1);
        }
    }

    // Render
    {

        GLint position = glGetAttribLocation(shader,
                                             "vertex_position");
        GLint uvs = glGetAttribLocation(shader, "vertex_uvs");

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
        GL_ERR;
        glUniformMatrix4fv(
                glGetUniformLocation(shader, "view_matrix"),
                1,
                GL_FALSE,
                view_matrix);
        GL_ERR;
        glUniformMatrix4fv(
                glGetUniformLocation(shader, "projection_matrix"),
                1,
                GL_FALSE, projection_matrix);

        GL_ERR;

        // Render text in quads
        int bytes_per_float = 4;
        int stride = bytes_per_float * 5;
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, d.vertex_data);
        glEnableVertexAttribArray(uvs);
        glVertexAttribPointer(uvs, 2, GL_FLOAT, GL_FALSE, stride, d.vertex_data + 3);
        glDrawArrays(GL_TRIANGLES, 0, len * 6);

        /*glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, d.texture);
        GL_ERR;
        int bytes_per_float = 4;
        int stride = bytes_per_float * (4);
        gl_error("Before vertex_arrays", __LINE__);
        glEnableVertexAttribArray(position);
        GL_ERR;
        glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE, stride, d.vertex_data);
        GL_ERR;
        glDrawArrays(GL_TRIANGLES, 0, d.max_text_length * 6);*/
        GL_ERR;
    }

}

#endif //BLOCKS_GP_FONT_RENDERING_H

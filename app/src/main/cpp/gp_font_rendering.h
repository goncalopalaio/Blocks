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

    stbtt_packedchar *font_char_data;
    int font_bitmap_width;
    int font_bitmap_height;
    int font_first_char;
} FontData;

FontData font_init(uint32_t font_size) {
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

    const char font_first_char = ' ';
    const int font_char_count = '~' - ' ';
    const int bitmap_width = 1024;
    const int bitmap_height = 512;
    const int oversample_x = 1;
    const int oversample_y = 1;

    log_fmt("font_init - font_first_char: %d", font_first_char);
    log_fmt("font_init - font_char_count: %d", font_char_count);

    auto *bitmap = (uint8_t *) malloc(
            sizeof(uint8_t) * bitmap_width * bitmap_height);
    auto font_char_data = (stbtt_packedchar *) malloc(
            sizeof(stbtt_packedchar) * font_char_count);

    stbtt_pack_context context;
    if (!stbtt_PackBegin(&context, bitmap, bitmap_width, bitmap_height, 0, 1,
                         NULL)) {
        log_str("failed to pack begin font");
        assert(0);
        return result;
    }
    stbtt_PackSetOversampling(&context, oversample_x, oversample_y);
    if (!stbtt_PackFontRange(&context, font_file, 0, font_size, font_first_char,
                             font_char_count, font_char_data)) {
        log_str("failed to pack font");
        assert(0);
        return result;
    }
    stbtt_PackEnd(&context);

    GLuint font_texture;
    // Upload texture data
    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bitmap_width, bitmap_height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, bitmap);
    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Cleanup
    free(bitmap);
    free(font_file);

    {
        result.texture = font_texture;
        result.max_text_length = 256;
        result.vertex_data_size = sizeof(float) * result.max_text_length * 6 * 4;

        result.font_char_data = font_char_data;
        result.font_bitmap_width = bitmap_width;
        result.font_bitmap_height = bitmap_height;
        result.font_first_char = font_first_char;

        result.vertex_data = (float *) malloc(result.vertex_data_size);
        memset(result.vertex_data, 0, result.vertex_data_size);
    }
    return result;
}

void font_render(FontData d, float x, float y, const char *text, int shader, float model_matrix[], float view_matrix[], float projection_matrix[]) {
    memset(d.vertex_data, 0, d.vertex_data_size);
    int len = M_MIN(d.max_text_length, strlen(text));
    //log_fmt("rendering %d characters", len);
    float* buf = d.vertex_data;
    for (int i = 0; i < len; i++) {
        char character = text[i];
        //log_fmt("reading %c character", character);

        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(d.font_char_data,
                            d.font_bitmap_width,
                            d.font_bitmap_height,
                            character - d.font_first_char,
                            &x,
                            &y,
                            &q,
                            1);

        buf = push_v4_arr(buf, q.s0, q.t0, q.x0, q.y0);
        buf = push_v4_arr(buf,q.s1, q.t0, q.x1, q.y0);
        buf = push_v4_arr(buf,q.s1, q.t1, q.x1, q.y1);
        buf = push_v4_arr(buf,q.s0, q.t0, q.x0, q.y0);
        buf = push_v4_arr(buf,q.s1, q.t1, q.x1, q.y1);
        buf = push_v4_arr(buf,q.s0, q.t1, q.x0, q.y1);
    }

    // Render
    {

        GLint position = glGetAttribLocation(shader,
                                             "vertex_position");
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
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, d.texture);
        GL_ERR;
        int bytes_per_float = 4;
        int stride = bytes_per_float * (4);
        gl_error("Before vertex_arrays", __LINE__);
        glEnableVertexAttribArray(position);
        GL_ERR;
        glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE, stride, d.vertex_data);
        GL_ERR;
        glDrawArrays(GL_TRIANGLES, 0, d.max_text_length * 2);
        GL_ERR;
    }

}

#endif //BLOCKS_GP_FONT_RENDERING_H

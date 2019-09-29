//
// Created by Gonçalo Palaio on 2019-09-28.
//

#ifndef BLOCKS_GP_FONT_RENDERING_H
#define BLOCKS_GP_FONT_RENDERING_H

#include <cstdio>

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

void font_init(uint32_t _text_max_length, uint32_t _font_size, uint32_t _atlas_width,
               uint32_t _atlas_height,
               uint32_t _oversample_x, uint32_t _oversample_y, LoadedFont *f, TextData *d) {
    f->font_size = _font_size;
    f->font_texture = 0;
    f->font_first_char = ' ';
    f->font_char_count = '~' - ' ';
    f->font_oversample_x = _oversample_x;
    f->font_oversample_y = _oversample_y;
    f->font_atlas_width = _atlas_width;
    f->font_atlas_height = _atlas_height;

    d->elems_per_vertex = 3 + 2;
    d->cur_text_length = 0;
    d->max_text_length = _text_max_length;
    d->vertex_data_size = sizeof(float) * d->max_text_length * 6 * d->elems_per_vertex;
    d->vertex_data = (float *) malloc(d->vertex_data_size);
    memset(d->vertex_data, 0, d->vertex_data_size);

    // Prepare font
    auto *font_buffer = reinterpret_cast<unsigned char *>(read_entire_file("cmunrm.ttf",
                                                                           'r'));

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, font_buffer, 0)) {
        log_str("init font failed");
    } else {
        log_str("font loaded");
    }

    stbtt_GetFontVMetrics(&info, &f->font_ascent, NULL, NULL);

    auto *atlas_data = (uint8_t *) malloc(
            sizeof(uint8_t) * f->font_atlas_width * f->font_atlas_height);
    f->font_char_info = (stbtt_packedchar *) malloc(
            sizeof(stbtt_packedchar) * f->font_char_count);

    stbtt_pack_context context;
    if (!stbtt_PackBegin(&context, atlas_data, f->font_atlas_width, f->font_atlas_height, 0, 1,
                         nullptr)) {
        log_str("failed to pack begin font");
        assert(0);
        return;
    }

    stbtt_PackSetOversampling(&context, f->font_oversample_x, f->font_oversample_y);
    if (!stbtt_PackFontRange(&context, font_buffer, 0, f->font_size, f->font_first_char,
                             f->font_char_count, f->font_char_info)) {
        log_str("failed to pack font");
        assert(0);
        return;
    }
    stbtt_PackEnd(&context);

    // Upload texture data
    glGenTextures(1, &f->font_texture);
    glBindTexture(GL_TEXTURE_2D, f->font_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, f->font_atlas_width, f->font_atlas_height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, atlas_data);
    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Cleanup
    free(atlas_data);
    free(font_buffer);
}

void font_prepare_text(LoadedFont *f, TextData *d, const char *text, float *buffer,
                       int font_max_length) {
    // @TODO move allocations outside of render loop
    d->cur_text_length = strlen(text);
    assert(font_max_length > d->cur_text_length);
    log_fmt("cur_text_length: %d", d->cur_text_length);

    float scale = 0.2;

    float x =0;
    float y = 0;

    float left = 0;
    float top = 0;

    float fx = 0;
    float fy = 0;

    float *bf = buffer;
    for (int i = 0; i < d->cur_text_length; i++) {
        char character = text[i];
        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(f->font_char_info,
                            f->font_atlas_width,
                            f->font_atlas_height,
                            character - ' ',
                            &fx,
                            &fy,
                            &q,
                            0);

        float3 va,vb,vc,vd,ve,vf;
        float2 na,nb,nc,nd,ne,nf;

        set_float3(&va, q.x0 * scale, q.y0 * scale, 0); set_float2(&na, q.s0, q.t0);
        set_float3(&vb, q.x1 * scale, q.y0 * scale, 0); set_float2(&nb, q.s1, q.t0);
        set_float3(&vc, q.x1 * scale, q.y1 * scale, 0); set_float2(&nc, q.s1, q.t1);
        set_float3(&vd, q.x0 * scale, q.y0 * scale, 1); set_float2(&nd, q.s0, q.t0);
        set_float3(&ve, q.x1 * scale, q.y1 * scale, 1); set_float2(&ne, q.s1, q.t1);
        set_float3(&vf, q.x0 * scale, q.y1 * scale, 1); set_float2(&nf, q.s0, q.t1);

        bf = push_triangle_v3v2_arr(bf, &va, &na, &vb, &nb, &vc, &nc);
        bf = push_triangle_v3v2_arr(bf, &vd, &nd, &ve, &ne, &vf, &nf);

    /*    float3 tl;
        float2 n_tl;
        set_float3(&tl, left + scale * ( q.x0 - left ), top + scale * ( q.y0 - y ), 1 );
        set_float2(&n_tl,q.s0, q.t0 );

        float3 tr;
        float2 n_tr;
        set_float3(&tr, left + scale * ( q.x1 - left ), top + scale * ( q.y0 - y ), 1 );
        set_float2(&n_tr,q.s1, q.t0 );


        float3 bl;
        float2 n_bl;
        set_float3(&bl, left + scale * ( q.x0 - left ), top + scale * ( q.y1 - y ), 0 );
        set_float2(&n_bl,q.s0, q.t1 );


        float3 br;
        float2 n_br;
        set_float3(&br, left + scale * ( q.x1 - left ), top + scale * ( q.y1 - y ), 0 );
        set_float2(&n_br,q.s1, q.t1 );

        bf = push_triangle_v3v2_arr(bf, &tr, &n_tr, &tl, &n_tl, &bl, &n_bl);
        bf = push_triangle_v3v2_arr(bf, &bl, &n_bl, &br, &n_br, &tr, &n_tr);*/
    }

}

void font_render(GLuint shader, GLuint font_texture, TextData *text_data, float model_matrix[],
                 float view_matrix[], float projection_matrix[]) {

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
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    GL_ERR;
    int bytes_per_float = 4;
    int stride = bytes_per_float * (3 + 2);
    gl_error("Before vertex_arrays", __LINE__);
    glEnableVertexAttribArray(position);
    GL_ERR;
    glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, text_data->vertex_data);
    GL_ERR;
    glEnableVertexAttribArray(uvs);
    GL_ERR;
    glVertexAttribPointer(uvs, 2, GL_FLOAT, GL_FALSE, stride, text_data->vertex_data + 3);
    GL_ERR;
    int draw_until = text_data->max_text_length * 6;
    //log_fmt("data_size: %d curr_text_length: %d drawing until: %d", text_data->vertex_data_size, text_data->cur_text_length, draw_until);
    glDrawArrays(GL_TRIANGLES, 0, draw_until);
    GL_ERR;
}

#endif //BLOCKS_GP_FONT_RENDERING_H

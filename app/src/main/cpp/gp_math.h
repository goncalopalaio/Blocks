//
// Created by Gonçalo Palaio on 2019-09-28.
//

#ifndef BLOCKS_GP_MATH_H
#define BLOCKS_GP_MATH_H

#include "gp_platform.h"

float f3_length(float3 *src) {
    return sqrtf((src)->x * (src)->x + (src)->y * (src)->y + (src)->z * (src)->z);
}

void f3_ip_normalize(float3 *src) {
    float l = f3_length(src);
    if (l > 0) {
        l = 1.0f / l;
        src->x = src->x * l;
        src->y = src->y * l;
        src->z = src->z * l;
    } else {
        src->x = src->y = src->z = 0.0f;
    }
}

void set_float3(float3 *v, float x, float y, float z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

void set_float4(float4 *v, float x, float y, float z, float w) {
    v->x = x;
    v->y = y;
    v->z = z;
    v->w = w;
}

void set_float2(float2 *v, float x, float y) {
    v->x = x;
    v->y = y;
}

void log_float3(const char *heading, float3 *v) {
    log_fmt("%s x: %f y: %f z: %f", heading, v->x, v->y, v->z);
}

void log_float2(const char *heading, float2 *v) {
    log_fmt("%s x: %f y: %f", heading, v->x, v->y);
}

inline float *push_v3_arr(float *v, float x, float y, float z) {
    v[0] = x;
    v[1] = y;
    v[2] = z;

    return v + 3;
}

inline float *push_v4_arr(float *v, float x, float y, float z, float a) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = a;
    //log_fmt("push_v4_arr: %f, %f, %f, %f",  x, y, z, a);
    return v + 4;
}

inline float *push_v2_arr(float *v, float x, float y) {
    v[0] = x;
    v[1] = y;
    return v + 2;
}

inline float *push_v5_arr(float *v, float x, float y, float z, float s, float t) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = s;
    v[4] = t;
    printf("push_v5_arr: %f, %f, %f, %f, %f\n", x, y, z, s, t);
    return v + 5;
}

inline float *push_v6_arr(float *v, float x, float y, float z, float x1, float y1, float z1) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = x1;
    v[4] = y1;
    v[5] = z1;
    return v + 6;
}

inline float *push_v1_arr(float *v, float x) {
    v[0] = x;
    return v + 1;
}

float *push_textured_quad_arr(float *v, float x0, float y0, float x1, float y1, float s0, float t0,
                              float s1, float t1) {
    // quad 1 2 3 4
    // -->
    // triangle 1 2 3
    // triangle 3 4 1

    //v = push_v5_arr(v, x0, y0, 0, s0, t0); // 1
    //v = push_v5_arr(v, x1, y0, 0, s1, t0); // 2
    //v = push_v5_arr(v, x1, y1, 0, s1, t1); // 3
    //v = push_v5_arr(v, x0, y1, 0, s0, t1); // 4

    v = push_v5_arr(v, x0, y0, 1.0, s0, t0); // 1
    v = push_v5_arr(v, x1, y0, 1.0, s1, t0); // 2
    v = push_v5_arr(v, x1, y1, 1.0, s1, t1); // 3

    v = push_v5_arr(v, x1, y1, 1.0, s1, t1); // 3
    v = push_v5_arr(v, x0, y1, 1.0, s0, t1); // 4
    v = push_v5_arr(v, x0, y0, 1.0, s0, t0); // 1

    return v;
}

float *
push_textured_quad_scaled_arr(float *v, float x0, float y0, float x1, float y1, float s0, float t0,
                              float s1, float t1, float scale_x, float scale_y) {
    return push_textured_quad_arr(v, x0 * scale_x, y0 * scale_y, x1 * scale_x, y1 * scale_y, s0, t0,
                                  s1, t1);
}

#endif //BLOCKS_GP_MATH_H

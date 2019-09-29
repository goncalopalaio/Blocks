//
// Created by Gonçalo Palaio on 2019-09-28.
//

#ifndef BLOCKS_GP_MATH_H
#define BLOCKS_GP_MATH_H

#include "gp_platform.h"

void set_float3(float3 *v, float x, float y, float z) {
    v->x = x;
    v->y = y;
    v->z = z;
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

    return v + 4;
}

inline float *push_v2_arr(float *v, float x, float y) {
    v[0] = x;
    v[1] = y;
    return v + 2;
}

inline float *
push_triangle_v3v2_arr(float *dest, float3 *va, float2 *na, float3 *vb, float2 *nb, float3 *vc,
                       float2 *nc) {
    dest = push_v3_arr(dest, va->x, va->y, va->z);
    dest = push_v2_arr(dest, na->x, na->y);

    dest = push_v3_arr(dest, vb->x, vb->y, vb->z);
    dest = push_v2_arr(dest, nb->x, nb->y);

    dest = push_v3_arr(dest, vc->x, vc->y, vc->z);
    dest = push_v2_arr(dest, nc->x, nc->y);

    return dest;
}


#endif //BLOCKS_GP_MATH_H

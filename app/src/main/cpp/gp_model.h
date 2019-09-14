//
// Created by Gonçalo Palaio on 2019-09-13.
//

#ifndef BLOCKS_GP_MODEL_H
#define BLOCKS_GP_MODEL_H

typedef struct {
    bool has_normals;
    int elems_per_normal;
    bool has_uvs;
    int elems_per_uvs;
    int elems_per_vertex;
    int vertex_number;
    int elems_stride;
    int size;
    float* data;

} SModelData;

SModelData parse_smodel_file(char* file_data);

#endif //BLOCKS_GP_MODEL_H

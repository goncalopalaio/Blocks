//
// Created by Gonçalo Palaio on 2019-09-14.
//
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "gp_model.h"

SModelData parse_smodel_file(char* file_data) {

    char* token = strtok(file_data, " \n");

    SModelData model;
    model.vertex_number = -1;
    model.data = nullptr;

    int number_elements_read = 0;

    // @NOTE For now we are assuming that there's a single model per file.

    do {
        if (strcmp(token, "?") == 0) {
            // Skip first header
            token = strtok(nullptr, " \n");
            //printf("Skipping: %s\n", token);
            token = strtok(nullptr, " \n");
            //printf("Skipping: %s\n", token);
            continue;

        }
        if (strcmp(token, ">") == 0) {
            token = strtok(nullptr, " \n");
            //printf("Reading section name: %s\n", token);

            token = strtok(nullptr, " \n");
            model.vertex_number = strtol(token, nullptr, 10);
            //printf("Vertex number: %d\n", model.vertex_number);

            token = strtok(nullptr, " \n");
            model.has_normals = strtol(token, nullptr, 10) != 0;
            //printf("Has normals: %d\n", model.has_normals);

            token = strtok(nullptr, " \n");
            model.has_uvs = strtol(token, nullptr, 10) != 0;
            //printf("Has uvs: %d\n", model.has_uvs);

            model.elems_per_vertex = 3;
            model.elems_per_normal = 0;
            model.elems_per_uvs = 0;
            if (model.has_normals) model.elems_per_normal = 3;
            if (model.has_uvs) model.elems_per_uvs = 2;

            model.elems_stride = model.elems_per_vertex + model.elems_per_normal + model.elems_per_uvs;

            model.size = model.vertex_number * model.elems_stride;
            //printf("Size: %d\n", model.size);
            model.data = (float*) malloc(sizeof(float) * (model.size));

            // Skipping bounding box information
            for (int i = 0; i < 6; ++i) strtok(nullptr, " \n");
            continue;
        }

        // @NOTE this could be faster, we know we are inside a float section wihout looping

        // ensure we found the previous header line to allocate the space
        assert(model.data);

        float f = strtof(token, nullptr);
        model.data[number_elements_read] = f;
        number_elements_read += 1;

    } while((token = strtok(nullptr, " \n")));

    //printf("Number of elements read: %d\n", number_elements_read);
    free(file_data);

    assert(model.vertex_number > 0);
    assert(model.data);
    return model;
}
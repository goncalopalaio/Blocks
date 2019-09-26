//
// Created by Gonçalo Palaio on 2019-09-14.
//
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#import "gp_platform.h"
#include "gp_model.h"

SModelData parse_smodel_file_as_single_model(char* file_data) {

    char* token = strtok(file_data, " \n");

    SModelData model;
    model.vertex_number = -1;
    // @note assuming that we always have normals and uvs in the file
    model.elems_per_vertex = 3;
    model.elems_per_normal = 3;
    model.elems_per_uvs = 2;
    model.elems_stride = model.elems_per_vertex + model.elems_per_normal + model.elems_per_uvs;

    model.data = nullptr;

    int number_elements_read = 0;

    // @NOTE For now we are assuming that there's a single model per file.

    do {
        // ? 2.0 ../assets/model.obj 28539 17
        if (strcmp(token, "?") == 0) {
            token = strtok(nullptr, " \n");
            float export_version = strtof(token, nullptr);
            log_fmt("export version: %f", export_version);

            token = strtok(nullptr, " \n");
            log_fmt("model name: %s", token);

            token = strtok(nullptr, " \n");
            model.vertex_number = strtol(token, nullptr, 10);
            log_fmt("total vertices: %d", model.vertex_number);

            token = strtok(nullptr, " \n");
            log_fmt("total submodels: %s", token);

            assert(export_version == 2.0f);
            model.size = model.vertex_number * model.elems_stride;
            model.data = (float*) malloc(sizeof(float) * (model.size));
            continue;
        }

        // Sub model summary
        // % 36
        if (strcmp(token, "%") == 0) {
            // Skipping
            token = strtok(nullptr, " \n");
            log_fmt("(>) %s", token);

            continue;
        }

        // Sub model information
        // > submodel_name 1920 1 1 -0.597168 0.084459 0.106442 0.597168 0.38938 0.411364
        if (strcmp(token, ">") == 0) {
            // Skipping
            for (int i = 0; i < 10; ++i) {
                token = strtok(nullptr, " \n");
                log_fmt("(>) %s", token);
            }

            continue;
        }

        // @NOTE this could be faster, we know we are inside a float section without looping

        // ensure we found the previous header line to allocate the space
        assert(model.data);

        float f = strtof(token, nullptr);
        model.data[number_elements_read] = f;
        number_elements_read += 1;

    } while((token = strtok(nullptr, " \n")));

    log_fmt("Number of elements read: %d -> should have: %d\n", number_elements_read, (model.elems_stride * model.vertex_number));
    free(file_data);

    assert(model.vertex_number > 0);
    assert(model.data);
    return model;
}
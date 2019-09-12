//
// Created by Gonçalo Palaio on 2019-09-05.
//

#ifndef BLOCKS_GAME_H
#define BLOCKS_GAME_H

struct SavedState {
    float angle;
    int32_t x;
    int32_t y;
};

typedef struct {
    char *path;
    char *buffer;
    long size;
    bool loaded;
} Asset;

struct State {
    bool valid;
    bool paused;
    Asset *assets;
    int total_assets;

    int w;
    int h;

    // @TODO temporary fields? I think they don't need to be shared like this
    GLuint main_shader_program;
    GLint main_position_handle;
    float grey;
};

State init_state_game();

void on_resources_loaded_game(Asset *assets, int total_assets);

void unload_resources_game(Asset *assets, int total_assets);

void init_game(State *state, int w, int h);

void render_game(State *state);

#endif //BLOCKS_GAME_H

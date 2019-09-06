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

struct State {
    bool paused;
    GLuint main_shader_program;
    GLint main_position_handle;

    bool valid;
    int w;
    int h;
    float grey;
};

State init_game(int w, int h);
void render_game(State *state);

#endif //BLOCKS_GAME_H

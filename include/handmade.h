//
// Created by Alexey Logachev on 23.09.2025.
//

#ifndef HANDMADE_HERO_HANDMADE_H
#define HANDMADE_HERO_HANDMADE_H

struct game_offscreen_buffer {
    void *memory;
    int width;
    int height;
    int pitch;
};

internal void game_update_and_render(struct game_offscreen_buffer *buffer, int blue_offset, int green_offset);


#endif //HANDMADE_HERO_HANDMADE_H
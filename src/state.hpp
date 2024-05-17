#pragma once

#include "shape.hpp"

#include <cglm/types.h>

template < typename T > void array_swap_last( T * arr, int count, int index )
{
    arr[ index ] = arr[ count - 1 ];
}

struct state_t {
    float tick_time;
    float render_time;
    float tick_step;
    float render_step;

    rect_t * room_rect_list;
    int room_count;

    vec2 player_pos;
    float player_z;
    float player_vel_z;
};

extern state_t state;

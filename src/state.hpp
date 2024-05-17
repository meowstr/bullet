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

    vec2 player_pos;
};

extern state_t state;

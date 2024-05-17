#pragma once

#include "shape.hpp"

#include <cglm/types.h>
#include <string.h>

template < typename T > void array_swap_last( T * arr, int count, int index )
{
    memcpy( arr + index, arr + count - 1, sizeof( T ) );
}

static const float k_hammer_length = 38.0f;

enum scene_t {
    SCENE_START,
    SCENE_GAME,
    SCENE_WIN,
    SCENE_LOSE,
};

struct state_t {
    float tick_time;
    float render_time;
    float tick_step;
    float render_step;

    rect_t * room_rect_list;
    int room_count;

    vec2 * bullet_old_pos_list;
    vec2 * bullet_pos_list;
    int bullet_count;

    vec2 * line_bullet_pos1_list;
    vec2 * line_bullet_pos2_list;
    vec2 * line_bullet_vel_list;
    int line_bullet_count;

    vec2 * mob_pos_list;
    int mob_count;

    vec2 player_pos;
    float player_z;
    float player_vel_z;

    float player_hammer;
    float player_hammer_vel;

    float fast_swing_timer;
    float fast_swing_vel;
    float fast_swing_cooldown;

    float funny_timer;

    scene_t scene;

    vec2 exit_pos;
};

extern state_t state;

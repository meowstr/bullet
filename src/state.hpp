#pragma once

#include "shape.hpp"

template < typename T > void array_swap_last( T * arr, int count, int index )
{
    arr[ index ] = arr[ count - 1 ];
}

const rect_t k_rect_enemy( 85, 5, 100, 100 );

const rect_t k_rect2( 1, 129, 268, 152 );

const rect_t k_rect3( 121, 329, 30, 30 );

const rect_t k_rect4( 88, 362, 30, 30 );
const rect_t k_rect5( 121, 362, 30, 30 );
const rect_t k_rect6( 154, 362, 30, 30 );

const rect_t k_rect7( 121, 395, 30, 30 );

const rect_t k_rect_ability1( 81, 441, 34, 34 );
const rect_t k_rect_ability2( 118, 441, 34, 34 );
const rect_t k_rect_ability3( 155, 441, 34, 34 );

const rect_t k_rect_remnant1( 88, 289, 30, 30 );
const rect_t k_rect_remnant2( 121, 289, 30, 30 );
const rect_t k_rect_remnant3( 154, 289, 30, 30 );

const rect_t k_rect_skill_button{ 270 - 32 - 10, 480 - 36 - 10, 32, 36 };

const rect_t k_rect_player( 122, 219, 26, 58 );

struct skillset_t {
    int health;
    int energy;
    int attack;
    int critical;
    int defense;
    int speed;
    int ward;
    int evade;
};

struct effectset_t {
    int fatigue;
    int slumber;
    int poison;
    int cryo;
    int pyro;
    int voltage;
    int fear;
    int weakness;
    int stun;
    int confusion;
};

struct character_t {
    skillset_t skillset;
    effectset_t effectset;

    int health;
    float accum;

    float poison_damage_timer;

    float stun_timer;

    // timers
    float fatigue_stack[ 32 ];
    float poison_stack[ 32 ];
    float cryo_stack[ 32 ];
    float pyro_stack[ 32 ];
    float voltage_stack[ 32 ];
    float fear_stack[ 32 ];
    float confusion_stack[ 32 ];
};

enum screen_t {
    SCREEN_BATTLE,
    SCREEN_SKILLS,
    SCREEN_PAUSE,
};

struct state_t {
    float tick_time;
    float render_time;

    float tick_step;
    float render_step;

    int last_damage;

    int touch;

    rect_t * pickup_rect_list;
    int * pickup_type_list;
    int * pickup_point_list;
    int pickup_count;

    character_t player;
    character_t enemy;

    screen_t screen;
};

extern state_t state;

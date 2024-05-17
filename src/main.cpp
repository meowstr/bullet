#include "audio.hpp"
#include "hardware.hpp"
#include "logging.hpp"
#include "render.hpp"
#include "state.hpp"
#include "utils.hpp"

#include <cglm/vec2.h>

#include <float.h>
#include <math.h>

static float clamp( float x, float min, float max )
{
    if ( x < min ) return min;
    if ( x > max ) return max;
    return x;
}

static void init_room_table()
{
    int cap = 32;
    state.room_rect_list = new rect_t[ cap ];
}

static int find_room( vec2 pos )
{
    int room = -1;
    for ( int i = 0; i < state.room_count; i++ ) {
        if ( state.room_rect_list[ i ].contains( pos[ 0 ], pos[ 1 ] ) ) {
            room = i;
        }
    }

    return room;
}

static void move( vec2 pos, vec2 step )
{
    vec2 new_pos;
    glm_vec2_add( pos, step, new_pos );

    int room = find_room( pos );
    int new_room = find_room( new_pos );

    glm_vec2_copy( new_pos, pos );

    if ( room == -1 ) return;     // already outside
    if ( new_room != -1 ) return; // inside

    // clamp to last good room
    rect_t r = state.room_rect_list[ room ];
    pos[ 0 ] = clamp( pos[ 0 ], r.x, r.x + r.w );
    pos[ 1 ] = clamp( pos[ 1 ], r.y, r.y + r.h );
}

static void tick_player()
{
    float speed = 200.0f;

    vec2 step;
    step[ 0 ] = hardware_x_axis();
    step[ 1 ] = hardware_y_axis();
    glm_vec2_normalize( step );
    glm_vec2_scale( step, speed * state.tick_step, step );

    move( state.player_pos, step );

    // jumping
    state.player_vel_z -= 9.8 * state.tick_step;
    state.player_z += state.player_vel_z * state.tick_step;
    if ( state.player_z < 0.0f ) {
        state.player_z = 0.0f;
        state.player_vel_z = 0.0f;
    }
}

static void jump()
{
    state.player_vel_z = 2.0f;
}

static void loop()
{
    float time = hardware_time();

    state.tick_step = fmin( time - state.tick_time, 1 / 60.0f );
    state.render_step = state.tick_step;

    state.tick_time = time;
    state.render_time = time;

    int event_count;
    int * events = hardware_events( &event_count );

    for ( int i = 0; i < event_count; i++ ) {
        if ( events[ i ] == EVENT_JUMP ) jump();
    }

    audio_tick();

    tick_player();

    render();
}

static void setup_rooms()
{
    int i = state.room_count++;
    state.room_rect_list[ i ].x = 0;
    state.room_rect_list[ i ].y = 0;
    state.room_rect_list[ i ].w = hardware_width();
    state.room_rect_list[ i ].h = hardware_height();
    state.room_rect_list[ i ].margin( 100 );

    i = state.room_count++;
    state.room_rect_list[ i ].x = hardware_width() * 0.5f;
    state.room_rect_list[ i ].y = 200;
    state.room_rect_list[ i ].w = hardware_width();
    state.room_rect_list[ i ].h = hardware_height() * 0.5f;
    state.room_rect_list[ i ].margin( 100 );
}

static void init()
{
    init_room_table();

    setup_rooms();

    state.player_pos[ 0 ] = 500;
    state.player_pos[ 1 ] = 500;
}

int main()
{
    INFO_LOG( "meow" );

    hardware_init();

    init();

    audio_init();

    audio_play_damage();

    render_init();

    hardware_set_loop( loop );

    audio_destroy();

    hardware_destroy();

    return 0;
}

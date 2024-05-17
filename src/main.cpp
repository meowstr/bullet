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

static void init_bullet_table()
{
    int cap = 1024;
    state.bullet_pos_list = new vec2[ cap ];
    state.bullet_old_pos_list = new vec2[ cap ];
}

static float room_metric( rect_t r, vec2 pos )
{
    // is this fucked? maybe....
    float dx = abs( pos[ 0 ] - ( r.x + r.w * 0.5f ) ) - r.w * 0.5f;
    float dy = abs( pos[ 1 ] - ( r.y + r.h * 0.5f ) ) - r.h * 0.5f;

    if ( dx < 0.0f ) dx = 0.0f;
    if ( dy < 0.0f ) dy = 0.0f;

    return dx + dy;
}

static int find_closest_room( float * out_metric, vec2 pos )
{
    int best_room = -1;
    float best_metric = 0.0f;
    for ( int i = 0; i < state.room_count; i++ ) {
        float metric = room_metric( state.room_rect_list[ i ], pos );
        if ( best_room == -1 || metric < best_metric ) {
            best_room = i;
            best_metric = metric;
        }
    }

    *out_metric = best_metric;
    return best_room;
}

static int constrain_to_rooms( vec2 pos )
{
    float metric;
    int room = find_closest_room( &metric, pos );

    // clamp to room
    rect_t r = state.room_rect_list[ room ];
    pos[ 0 ] = clamp( pos[ 0 ], r.x, r.x + r.w );
    pos[ 1 ] = clamp( pos[ 1 ], r.y, r.y + r.h );

    return metric != 0.0f;
}

static void tick_bullet( int i )
{
    vec2 step;
    glm_vec2_sub(
        state.bullet_pos_list[ i ],
        state.bullet_old_pos_list[ i ],
        step
    );
    glm_vec2_copy( state.bullet_pos_list[ i ], state.bullet_old_pos_list[ i ] );
    glm_vec2_add(
        state.bullet_pos_list[ i ],
        step,
        state.bullet_pos_list[ i ]
    );

    int collide = constrain_to_rooms( state.bullet_pos_list[ i ] );
}

static void tick_player()
{
    float speed = 200.0f;

    vec2 step;
    step[ 0 ] = hardware_x_axis();
    step[ 1 ] = hardware_y_axis();
    glm_vec2_normalize( step );
    glm_vec2_muladds( step, speed * state.tick_step, state.player_pos );

    constrain_to_rooms( state.player_pos );

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
    audio_play_jump();
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

    for ( int i = 0; i < state.bullet_count; i++ ) {
        tick_bullet( i );
    }

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

static void setup_bullets()
{
    for ( int i = 0; i < 1000; i++ ) {
        int j = state.bullet_count++;
        state.bullet_pos_list[ j ][ 0 ] = hardware_width() * 0.5f;
        state.bullet_pos_list[ j ][ 1 ] = hardware_height() * 0.5f;

        float t = rand() % 1000 / 1000.0f;
        t *= 2 * M_PI;
        vec2 step;
        step[ 0 ] = cos( t );
        step[ 1 ] = sin( t );

        glm_vec2_copy(
            state.bullet_pos_list[ j ],
            state.bullet_old_pos_list[ j ]
        );
        glm_vec2_muladds(
            step,
            ( rand() % 100 ) * ( 1 / 60.0f ),
            state.bullet_old_pos_list[ j ]
        );
    }
}

static void init()
{
    init_room_table();
    init_bullet_table();

    setup_rooms();
    setup_bullets();

    state.player_pos[ 0 ] = 500;
    state.player_pos[ 1 ] = 500;

    INFO_LOG( "bullet count %d", state.bullet_count );
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

#include "audio.hpp"
#include "hardware.hpp"
#include "logging.hpp"
#include "render.hpp"
#include "state.hpp"
#include "utils.hpp"

#include <cglm/vec2.h>

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
    int cap = 4096;
    state.bullet_pos_list = new vec2[ cap ];
    state.bullet_old_pos_list = new vec2[ cap ];
}

static void init_line_bullet_table()
{
    int cap = 4096;
    state.line_bullet_pos1_list = new vec2[ cap ];
    state.line_bullet_pos2_list = new vec2[ cap ];
    state.line_bullet_vel_list = new vec2[ cap ];
}

static void init_mob_table()
{
    int cap = 1024;
    state.mob_pos_list = new vec2[ cap ];
}

static void remove_mob( int i )
{
    array_swap_last( state.mob_pos_list, state.mob_count, i );
    state.mob_count--;
}

static void remove_bullet( int i )
{
    array_swap_last( state.bullet_pos_list, state.bullet_count, i );
    array_swap_last( state.bullet_old_pos_list, state.bullet_count, i );
    state.bullet_count--;
}

static void init_tables()
{
    init_room_table();
    init_bullet_table();
    init_line_bullet_table();
    init_mob_table();
}

static float room_metric( rect_t r, vec2 pos )
{
    // is this fucked? maybe....
    float dx = fabsf( pos[ 0 ] - ( r.x + r.w * 0.5f ) ) - r.w * 0.5f;
    float dy = fabsf( pos[ 1 ] - ( r.y + r.h * 0.5f ) ) - r.h * 0.5f;

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

static void spawn( vec2 pos )
{
    for ( int i = 0; i < 32; i++ ) {
        int j = state.bullet_count++;
        state.bullet_pos_list[ j ][ 0 ] = pos[ 0 ];
        state.bullet_pos_list[ j ][ 1 ] = pos[ 1 ];

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
            ( 100.0f + ( rand() % 100 ) ) * ( 1 / 60.0f ),
            state.bullet_old_pos_list[ j ]
        );
    }
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

    vec2 pre_constrained;
    glm_vec2_copy( state.bullet_pos_list[ i ], pre_constrained );
    int collide = constrain_to_rooms( state.bullet_pos_list[ i ] );

    if ( !collide ) return;

    // remove_bullet( i );

    // bounce
    if ( pre_constrained[ 0 ] - state.bullet_pos_list[ i ][ 0 ] != 0.0f ) {
        step[ 0 ] *= -1.0f;
    }
    if ( pre_constrained[ 1 ] - state.bullet_pos_list[ i ][ 1 ] != 0.0f ) {
        step[ 1 ] *= -1.0f;
    }
    glm_vec2_sub(
        state.bullet_pos_list[ i ],
        step,
        state.bullet_old_pos_list[ i ]
    );
}

static void tick_line_bullet( int i )
{
    glm_vec2_muladds(
        state.line_bullet_vel_list[ i ],
        state.tick_step,
        state.line_bullet_pos1_list[ i ]
    );

    glm_vec2_muladds(
        state.line_bullet_vel_list[ i ],
        state.tick_step,
        state.line_bullet_pos2_list[ i ]
    );

    constrain_to_rooms( state.line_bullet_pos1_list[ i ] );
    constrain_to_rooms( state.line_bullet_pos2_list[ i ] );
}

static void tick_hammer()
{
    vec2 hammer_end;
    glm_vec2_copy( state.player_pos, hammer_end );
    hammer_end[ 0 ] += k_hammer_length * cos( state.player_hammer );
    hammer_end[ 1 ] += k_hammer_length * sin( state.player_hammer );

    float mob_hit_distance = 20.0f;

    if ( state.player_z != 0.0f ) return; // player is in air

    for ( int i = 0; i < state.mob_count; i++ ) {
        if ( glm_vec2_distance2( hammer_end, state.mob_pos_list[ i ] ) <
                 mob_hit_distance * mob_hit_distance &&
             ( state.player_hammer_vel > 4.0f || state.player_hammer_vel < -4.0f
             ) ) { // AL: Check hammer velocity before doing damage to enemy

            // cancel fast swing
            if ( state.fast_swing_timer ) {
                state.fast_swing_timer = 0.0f;
            }

            spawn( state.mob_pos_list[ i ] );
            remove_mob( i );
            state.player_hammer_vel = 0.0f;
            trigger_camera_shake();
            audio_play_damage();
            break;
        }
    }
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

    if ( tick_timer( &state.fast_swing_timer, state.tick_step ) ) {
        state.player_hammer_vel = state.fast_swing_vel;
    }

    state.player_hammer += state.player_hammer_vel * state.tick_step;
    state.player_hammer_vel *= 0.99f;
}

static void jump()
{
    state.player_vel_z = 2.0f;
    audio_play_jump();
}

static void spawn_funnies( vec2 pos )
{
    for ( int i = 0; i < 25; i++ ) {
        int j = state.bullet_count++;
        state.bullet_pos_list[ j ][ 0 ] = pos[ 0 ];
        state.bullet_pos_list[ j ][ 1 ] = pos[ 1 ];

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

static void do_funny()
{
    int count = state.bullet_count;
    for ( int i = 0; i < count; i++ ) {
        spawn_funnies( state.bullet_pos_list[ i ] );
    }
    audio_play_damage();
}

static void tick()
{
    if ( state.funny_timer ) {
        state.funny_timer -= state.tick_step;
        if ( state.funny_timer <= 0.0f ) {
            do_funny();
            state.funny_timer = 0.0f;
        }
    }

    int event_count;
    int * events = hardware_events( &event_count );

    for ( int i = 0; i < event_count; i++ ) {
        if ( events[ i ] == EVENT_JUMP ) jump();
        if ( events[ i ] == EVENT_HAMMER_CW )
            state.player_hammer_vel += 10.0f * state.tick_step;
        if ( events[ i ] == EVENT_HAMMER_CCW )
            state.player_hammer_vel -= 10.0f * state.tick_step;

        if ( events[ i ] == EVENT_FAST_HAMMER_CW ) {
            state.fast_swing_timer = 0.2f;
            state.fast_swing_vel = 40.0f;
        }

        if ( events[ i ] == EVENT_FAST_HAMMER_CCW ) {
            state.fast_swing_timer = 0.2f;
            state.fast_swing_vel = -40.0f;
        }
    }

    audio_tick();

    for ( int i = state.bullet_count - 1; i >= 0; i-- ) {
        tick_bullet( i );
    }

    for ( int i = 0; i < state.line_bullet_count; i++ ) {
        tick_line_bullet( i );
    }

    tick_player();
    tick_hammer();

    if ( state.player_z == 0.0f ) { // only if player is on the ground
        for ( int i = 0; i < state.bullet_count; i++ ) {
            if ( glm_vec2_distance(
                     state.player_pos,
                     state.bullet_pos_list[ i ]
                 ) < 5.0f ) {
                state.scene = SCENE_LOSE;
            }
        }
    }

    if ( glm_vec2_distance( state.player_pos, state.exit_pos ) < 50.0f ) {
        state.scene = SCENE_WIN;
    }
}

static void loop()
{
    float time = hardware_time();

    state.tick_step = fminf( time - state.tick_time, 1 / 60.0f );
    state.render_step = state.tick_step;

    state.tick_time = time;
    state.render_time = time;

    if ( state.scene == SCENE_GAME ) {
        tick();
    }

    if ( state.scene == SCENE_START ) {
        int event_count;
        int * events = hardware_events( &event_count );

        for ( int i = 0; i < event_count; i++ ) {
            if ( events[ i ] == EVENT_JUMP ) {
                audio_play_damage();
                state.scene = SCENE_GAME;
            }
        }
    }

    render();
}

static void setup_rooms()
{
    int i = state.room_count++;
    state.room_rect_list[ i ].x = 100;
    state.room_rect_list[ i ].y = 100;
    state.room_rect_list[ i ].w = 600;
    state.room_rect_list[ i ].h = 600;

    i = state.room_count++;
    state.room_rect_list[ i ].x = 400;
    state.room_rect_list[ i ].y = 200;
    state.room_rect_list[ i ].w = 800;
    state.room_rect_list[ i ].h = 100;

    i = state.room_count++;
    state.room_rect_list[ i ].x = 1100;
    state.room_rect_list[ i ].y = 0;
    state.room_rect_list[ i ].w = 600;
    state.room_rect_list[ i ].h = 1000;

    i = state.room_count++;
    state.room_rect_list[ i ].x = 1400;
    state.room_rect_list[ i ].y = -900;
    state.room_rect_list[ i ].w = 100;
    state.room_rect_list[ i ].h = 1000;

    state.exit_pos[ 0 ] = 1450;
    state.exit_pos[ 1 ] = -850;
}

static void setup_bullets()
{
    for ( int i = 0; i < 0; i++ ) {
        int j = state.line_bullet_count++;

        state.line_bullet_pos1_list[ j ][ 0 ] = 400;
        state.line_bullet_pos1_list[ j ][ 1 ] = 400;
        state.line_bullet_pos2_list[ j ][ 0 ] = 500;
        state.line_bullet_pos2_list[ j ][ 1 ] = 500;
        state.line_bullet_vel_list[ j ][ 0 ] = -20;
        state.line_bullet_vel_list[ j ][ 1 ] = 20;
    }

    for ( int i = 0; i < 10; i++ ) {
        int j = state.mob_count++;

        state.mob_pos_list[ j ][ 0 ] = 100 + rand() % 400;
        state.mob_pos_list[ j ][ 1 ] = 100 + rand() % 400;
    }
}

static void init()
{
    init_tables();

    setup_rooms();
    setup_bullets();

    state.player_pos[ 0 ] = 500;
    state.player_pos[ 1 ] = 500;

    state.funny_timer = 3.0f;
}

#if defined( _WIN32 ) and RELEASE
int WinMain()
#else
int main()
#endif
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

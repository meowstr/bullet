#include "audio.hpp"
#include "hardware.hpp"
#include "logging.hpp"
#include "render.hpp"
#include "state.hpp"
#include "utils.hpp"

#include <float.h>
#include <math.h>

static float clamp( float x, float min, float max )
{
    if ( x < min ) return min;
    if ( x > max ) return max;
    return x;
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
        //if ( events[ i ] == EVENT_TOUCH ) handle_touch();
    }

    audio_tick();

    render();
}

static void init()
{
}

int main()
{
    INFO_LOG( "meow" );

    hardware_init();

    init();

    audio_init();

    render_init();

    hardware_set_loop( loop );

    audio_destroy();

    hardware_destroy();

    return 0;
}

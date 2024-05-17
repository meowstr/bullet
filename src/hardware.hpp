#pragma once

using loop_function_t = void ( * )();

enum event_t {
    EVENT_TOUCH,
    EVENT_JUMP,
    EVENT_HAMMER_CW,
    EVENT_HAMMER_CCW,
};

int hardware_init();
void hardware_destroy();
void hardware_set_loop( loop_function_t step );

int * hardware_events( int * out_count );

int hardware_width();
int hardware_height();

float hardware_time();

float hardware_x_axis();
float hardware_y_axis();

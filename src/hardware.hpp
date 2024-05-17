#pragma once

using loop_function_t = void ( * )();

enum event_t {
    EVENT_TOUCH,
};

int hardware_init();
void hardware_destroy();
void hardware_set_loop( loop_function_t step );

int * hardware_events( int * out_count );

int hardware_width();
int hardware_height();

float hardware_time();

void hardware_rumble();

int hardware_touch_down();

int hardware_touch_pos_x();
int hardware_touch_pos_y();

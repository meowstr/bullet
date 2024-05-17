#pragma once

#include "cglm/types.h"

struct color_t {
    float r;
    float g;
    float b;
};

const color_t color_white{ 1.0f, 1.0f, 1.0f };
const color_t color_black{ 0.0f, 0.0f, 0.0f };
const color_t color_gray{ 0.05f, 0.05f, 0.05f };
const color_t color_red{ 1.0f, 0.0f, 0.0f };
const color_t color_green{ 0.0f, 1.0f, 0.0f };
const color_t color_blue{ 0.0f, 0.0f, 1.0f };

const color_t color_yellow{ 1.0f, 1.0f, 0.5f };
const color_t color_orange{ 1.0f, 0.6f, 0.0f };

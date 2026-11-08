#pragma once

#include <lvgl.h>
#include "util.h"

/* Left (central) screen: base letter, layer "ladder", modifiers, connection, profile, battery.
 * Canvas is 32 x 128 (portrait) before rotation. */
void draw_layer_ladder(lv_obj_t *canvas, const struct status_state *state);

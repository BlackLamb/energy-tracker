#pragma once
#include <pebble.h>
#include "timer.h"

void timer_add_screen_init(void);
void timer_add_screen_show_new(void);
void timer_add_screen_show_edit(Timer* timer);
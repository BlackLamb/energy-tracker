#pragma once
#include <pebble.h>
#include "timer.h"

void timer_screen_init(void);
void timer_screen_set_timer(Timer *timer);
void timer_screen_show(void);
void timer_screen_destroy(void);
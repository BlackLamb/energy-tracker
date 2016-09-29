#pragma once

#include <pebble.h>
#include "timer.h"

void menu_draw_row_icon_text(GContext *ctx, char *text, GBitmap *icon, bool invert);
void timer_draw_row(Timer *timer, bool rate, bool center, GContext *ctx, bool invert);
void menu_draw_option(GContext *ctx, char *option, char *value, bool invert);
void uppercase(char *str);
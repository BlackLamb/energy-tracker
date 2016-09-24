#pragma once
#include "timer.h"

void main_screen_init(void);
void main_screen_show(void);
void main_screen_show_status_area(Timer *timer);
void main_screen_hide_status_area(bool finished);
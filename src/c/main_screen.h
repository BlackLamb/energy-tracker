#pragma once
#include "timer.h"

void main_screen_init(void);
void main_screen_show(void);
void main_screen_update_energy(void);
void main_screen_show_status_area(Timer *timer);
void main_screen_hide_status_area(bool finished);
Timer * main_screen_active_timer(void);
void main_screen_destroy(void);
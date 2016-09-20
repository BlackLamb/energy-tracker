#pragma once
#include "timer.h"

typedef void (*DurationCallback)(uint32_t duration);

void duration_screen_init(void);
void duration_screen_show(uint16_t duration, DurationCallback callback);
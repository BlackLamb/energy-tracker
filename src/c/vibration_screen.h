#pragma once

#include "timer.h"

typedef void (*VibrationCallback)(TimerVibration vibration);

void vibration_screen_init(void);
void vibration_screen_show(VibrationCallback callback, TimerVibration vibration);
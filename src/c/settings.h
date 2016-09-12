#pragma once

#include "timer.h"

#define SETTINGS_VERSION_CURRENT 1

typedef struct {
    bool timers_start_auto;
    TimerVibration timers_vibration;
    uint32_t timers_duration;
    bool timers_hours;
    bool show_clock;
} Settings;

Settings* settings();
void settings_load(void);
void settings_save(void);

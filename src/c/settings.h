#pragma once

#include "timer.h"

#define SETTINGS_VERSION_CURRENT 1

typedef struct {
		int8_t current_energy;
		int8_t max_energy;
		bool quicken_enabled;
		bool accel_enabled;
		uint8_t accel_tick;
		// TODO: Old setting to audit
    bool timers_start_auto;
    TimerVibration timers_vibration;
    uint32_t timers_duration;
    bool timers_hours;
    bool show_clock;
} Settings;

Settings* settings();
void settings_load(void);
void settings_save(void);

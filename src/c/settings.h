#pragma once

#include "timer.h"

#define SETTINGS_VERSION_CURRENT 1

typedef struct
{
  int8_t current_energy;
  int8_t max_energy;
  bool quicken_enabled;
  bool accel_enabled;
  uint8_t accel_tick;
  TimerVibration timers_tick_vibration;
  TimerVibration timers_finish_vibration;
  uint32_t timers_duration;
} Settings;

Settings *settings();
void settings_load(void);
void settings_save(void);

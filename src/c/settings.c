#include <pebble.h>
#include <@smallstoneapps/utils/macros.h>

#include "settings.h"

#include "timer.h"
#include "persist.h"

Settings _settings = {
    .current_energy = 10,
    .max_energy = 10,
    .quicken_enabled = false,
    .accel_enabled = true,
    .accel_tick = 3,
    .timers_tick_vibration = TIMER_VIBE_SHORT,
    .timers_finish_vibration = TIMER_VIBE_DOUBLE,
    .timers_duration = 5 * 60};

void settings_load(void)
{
    if (persist_exists(PERSIST_SETTINGS))
    {
        int res = persist_read_data(PERSIST_SETTINGS, &_settings, sizeof(_settings));
        if (res < 0)
        {
            LOG("Settings load failed: %d", res);
        }
    }
}

Settings *settings()
{
    return &_settings;
}

void settings_save(void)
{
    int res = persist_write_data(PERSIST_SETTINGS, &_settings, sizeof(_settings));
    if (res < 0)
    {
        LOG("Settings save failed: %d", res);
    }
    persist_write_int(PERSIST_SETTINGS_VERSION, SETTINGS_VERSION_CURRENT);
}
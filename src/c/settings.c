#include <pebble.h>
#include <@smallstoneapps/utils/macros.h>

#include "settings.h"

#include "timer.h"
#include "persist.h"

Settings _settings = {
    .timers_start_auto = false,
    .timers_vibration = TIMER_VIBE_SHORT,
    .timers_duration = 10 * 60,
    .timers_hours = false,
    .show_clock = false
};

void settings_load(void) {
    if (persist_exists(PERSIST_SETTINGS)) {
        int res = persist_read_data(PERSIST_SETTINGS, &_settings, sizeof(_settings));
        if (res < 0) {
            LOG("Settings load failed: %d", res);
        }
    }
}

Settings* settings() {
    return &_settings;
}

void settings_save(void) {
    int res  = persist_write_data(PERSIST_SETTINGS, &_settings, sizeof(_settings));
    if (res < 0) {
        LOG("Settings save failed: %d", res);
    }
    persist_write_int(PERSIST_SETTINGS_VERSION, SETTINGS_VERSION_CURRENT);
}
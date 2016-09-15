#include <pebble.h>
#include <@smallstoneapps/bitmap-loader/bitmap-loader.h>
#include "src/c/main_screen.h"
#include "timers.h"
#include "settings.h"
#include "persist.h"

static void init(void);
static void deinit(void);

int main(void) {
  init();
  app_event_loop();
  deinit();
}

static void init(void) {
  srand(time(NULL));
  timers_init();
  bitmaps_init();
  settings_load();
  timers_restore();
  main_screen_init();
  main_screen_show();
  main_screen_show_status_area();
  main_screen_hide_status_area();
}

static void deinit(void) {
  timers_save();
  settings_save();
  bitmaps_cleanup();
}
#include <pebble.h>
#include <@smallstoneapps/utils/macros.h>
#include <@smallstoneapps/bitmap-loader/bitmap-loader.h>
#include "src/c/main_screen.h"
#include "timers.h"
#include "settings.h"
#include "persist.h"

static void init(void);
static void deinit(void);

int main(void)
{
  init();
  app_event_loop();
  deinit();
}

static void init(void)
{
  srand(time(NULL));
  timers_init();
  bitmaps_init();
  settings_load();
  main_screen_init();
  main_screen_show();
  timers_restore();
}

static void deinit(void)
{
  timers_save();
  settings_save();
  bitmaps_cleanup();
}
#include <pebble.h>
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
  settings_load();
  timers_restore();
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit(void) {
  timers_save();
  settings_save();
}
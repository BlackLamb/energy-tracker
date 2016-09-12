#include <pebble.h>

#include <@smallstoneapps/utils/macros.h>

#include "../common.h"
#include "../timers.h"
#include "../settings.h"

#define MENU_SECTION_CLOCK 0
#define MENU_SECTION_TIMER 1
#define MENU_SECTION_OTHER 2

#define MENU_ROW_COUNT_OTHER 5

#define MENU_ROW_OTHER_ADD_TIMER 0
#define MENU_ROW_OTHER_ADD_STOPWATCH 1
#define MENU_ROW_OTHER_CONTROLS 2
#define MENU_ROW_OTHER_SETTINGS 3
#define MENU_ROW_OTHER_ABOUT 4

static void window_load(Window* window);
static void window_unload(Window* window);
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static uint16_t menu_num_sections(struct MenuLayer* menu, void* callback_context);
static uint16_t menu_num_rows(struct MenuLayer* menu, uint16_t *section_index, void* callback_context);
static int16_t menu_cell_height(struct MenuLayer* menu, MenuIndex* cell_index, void* callback_context);
static void menu_draw_row(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void* callback_context);
static void menu_draw_row_clock(GContext* ctx, const Layer* cell_layer);
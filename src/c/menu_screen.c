#include <pebble.h>
#include <@smallstoneapps/utils/macros.h>
#include <@smallstoneapps/bitmap-loader/bitmap-loader.h>
#include "common.h"
#include "timers.h"
#include "settings.h"
#include "icons.h"
#include "menu_screen.h"
#include "timer_add_screen.h"
#include "duration_screen.h"
#include "amount_screen.h"
#include "about_screen.h"
#include "settings_screen.h"
#include "vibration_screen.h"
#include "timer_screen.h"

#define MENU_SECTION_MODIFIERS 0
#define MENU_SECTION_TIMERS 1
#define MENU_SECTION_OTHER 2

#define MENU_ROW_COUNT_MODIFIERS 1

#define MENU_ROW_MODIFIERS_QUICKEN 0

#define MENU_ROW_COUNT_OTHER 3

#define MENU_ROW_OTHER_ADD_TIMER 0
#define MENU_ROW_OTHER_SETTINGS 1
#define MENU_ROW_OTHER_ABOUT 2

static void window_load(Window *window);
static void window_unload(Window *window);
static uint16_t menu_num_sections(struct MenuLayer *menu, void *callback_context);
static uint16_t menu_num_rows(struct MenuLayer *menu, uint16_t section_index, void *callback_context);
static int16_t menu_cell_height(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context);
static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void menu_draw_row_modifiers(GContext *ctx, const Layer *cell_layer, uint16_t row_index);
static void menu_draw_row_timers(GContext *ctx, const Layer *cell_layer, uint16_t row_index);
static void menu_draw_row_other(GContext *ctx, const Layer *cell_layer, uint16_t row_index);
static void menu_select(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context);
static void menu_select_modifiers(uint16_t row_index);
static void menu_select_timers(uint16_t row_index);
static void menu_select_other(uint16_t row_index);
static void menu_select_long(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context);
static void timers_update_handler(void);
static void timer_highlight_handler(Timer *timer);

static Window *s_window;
static MenuLayer *s_menu;

void menu_screen_init(void)
{
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = window_load,
                                           .unload = window_unload});
  timers_register_update_handler(timers_update_handler);
  timers_register_highlight_handler(timer_highlight_handler);
  // Register Sub Windows
  timer_add_screen_init();
  duration_screen_init();
  amount_screen_init();
  about_screen_init();
  settings_screen_init();
  vibration_screen_init();
  timer_screen_init();
}

void menu_screen_show(void)
{
  window_stack_push(s_window, true);
}

static void window_load(Window *window)
{
  s_menu = menu_layer_create_fullscreen(s_window);
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
                                             .get_num_sections = menu_num_sections,
                                             .get_num_rows = menu_num_rows,
                                             .get_cell_height = menu_cell_height,
                                             .draw_row = menu_draw_row,
                                             .select_click = menu_select,
                                             .select_long_click = menu_select_long,
                                         });
  menu_layer_add_to_window(s_menu, s_window);
}

static void window_unload(Window *window)
{
  DEBUG("Menu Window Unload");
  menu_layer_destroy_safe(s_menu);
}

static uint16_t menu_num_sections(struct MenuLayer *menu, void *callback_context)
{
  return 3;
}

static uint16_t menu_num_rows(struct MenuLayer *menu, uint16_t section_index, void *callback_context)
{
  switch (section_index)
  {
  case MENU_SECTION_MODIFIERS:
    return MENU_ROW_COUNT_MODIFIERS;
  case MENU_SECTION_TIMERS:
    return timers_count();
  case MENU_SECTION_OTHER:
    return MENU_ROW_COUNT_OTHER;
  default:
    return 0;
  }
}

static int16_t menu_cell_height(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context)
{
  switch (cell_index->section)
  {
  case MENU_SECTION_TIMERS:
    return 34;
  }
  return 32;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context)
{
  switch (cell_index->section)
  {
  case MENU_SECTION_MODIFIERS:
    menu_draw_row_modifiers(ctx, cell_layer, cell_index->row);
    break;
  case MENU_SECTION_TIMERS:
    menu_draw_row_timers(ctx, cell_layer, cell_index->row);
    break;
  case MENU_SECTION_OTHER:
    menu_draw_row_other(ctx, cell_layer, cell_index->row);
    break;
  }
}

static void menu_draw_row_modifiers(GContext *ctx, const Layer *cell_layer, uint16_t row_index)
{
  switch (row_index)
  {
  case MENU_ROW_MODIFIERS_QUICKEN:
    menu_draw_option(ctx, "Quicken", settings()->quicken_enabled ? "ON" : "OFF");
    break;
  default:
    break;
  }
}

static void menu_draw_row_timers(GContext *ctx, const Layer *cell_layer, uint16_t row_index)
{
  Timer *timer = timers_get(row_index);
  if (!timer)
  {
    return;
  }
  timer_draw_row(timer, true, false, ctx);
}

static void menu_draw_row_other(GContext *ctx, const Layer *cell_layer, uint16_t row_index)
{
  switch (row_index)
  {
  case MENU_ROW_OTHER_ADD_TIMER:
    menu_draw_row_icon_text(ctx, "Timer", bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS_16, ICON_RECT_ADD));
    break;
  case MENU_ROW_OTHER_SETTINGS:
    menu_draw_row_icon_text(ctx, "Settings", bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS_16, ICON_RECT_SETTINGS));
    break;
  case MENU_ROW_OTHER_ABOUT:
    menu_draw_row_icon_text(ctx, "About", bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS_16, ICON_RECT_ABOUT));
    break;
  }
}

static void menu_select(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context)
{
  switch (cell_index->section)
  {
  case MENU_SECTION_MODIFIERS:
    menu_select_modifiers(cell_index->row);
    break;
  case MENU_SECTION_TIMERS:
    menu_select_timers(cell_index->row);
    break;
  case MENU_SECTION_OTHER:
    menu_select_other(cell_index->row);
    break;
  }
}

static void menu_select_modifiers(uint16_t row_index)
{
  switch (row_index)
  {
  case MENU_ROW_MODIFIERS_QUICKEN:
    settings()->quicken_enabled = !settings()->quicken_enabled;
    menu_layer_reload_data(s_menu);
    break;
  default:
    break;
  }
}

static void menu_select_timers(uint16_t row_index)
{
  Timer *timer = timers_get(row_index);
  if (!timer)
  {
    return;
  }

  switch (timer->status)
  {
  case TIMER_STATUS_STOPPED:
  {
    timer_start(timer);
    break;
  }
  case TIMER_STATUS_RUNNING:
    timer_pause(timer);
    break;
  case TIMER_STATUS_PAUSED:
    timer_resume(timer);
    break;
  case TIMER_STATUS_DONE:
    timer_reset(timer, false);
    break;
  }
}

static void menu_select_other(uint16_t row_index)
{
  switch (row_index)
  {
  case MENU_ROW_OTHER_ADD_TIMER:
    timer_add_screen_show_new();
    break;
  case MENU_ROW_OTHER_ABOUT:
    about_screen_show();
    break;
  case MENU_ROW_OTHER_SETTINGS:
    settings_screen_show();
    break;
  }
}

static void menu_select_long(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context)
{
  if (cell_index->section == MENU_SECTION_TIMERS)
  {
    Timer *timer = timers_get(cell_index->row);
    timer_screen_set_timer(timer);
    timer_screen_show();
  }
}

static void timers_update_handler(void)
{
  if (s_menu) { menu_layer_reload_data(s_menu); }
}

static void timer_highlight_handler(Timer *timer)
{
  uint16_t index = timers_index_of(timer->id);
  menu_layer_set_selected_index(s_menu, (MenuIndex){.section = 1, .row = index}, MenuRowAlignCenter, true);
}
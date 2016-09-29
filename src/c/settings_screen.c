#include <pebble.h>
#include <@smallstoneapps/utils/macros.h>
#include <@smallstoneapps/bitmap-loader/bitmap-loader.h>

#include "settings_screen.h"
#include "duration_screen.h"
#include "amount_screen.h"
#include "vibration_screen.h"

#include "settings.h"
#include "timers.h"
#include "common.h"

#define MENU_NUM_SECTIONS 2
#define MENU_SECTION_CHARACTER 0
#define MENU_SECTION_TIMERS 1

#define MENU_SECTION_ROWS_CHARACTER 1
#define MENU_SECTION_ROWS_TIMERS 5

#define MENU_ROW_CHARACTER_ENERGY 0

#define MENU_ROW_TIMERS_DURATION 0
#define MENU_ROW_TIMERS_ACCEL_ENABLE 1
#define MENU_ROW_TIMERS_ACCEL_TICK 2
#define MENU_ROW_TIMERS_VIBRATION_TICK 3
#define MENU_ROW_TIMERS_VIBRATION_FULL 4

static void window_load(Window *window);
static void window_unload(Window *window);
static uint16_t menu_get_num_sections_callback(MenuLayer *me, void *data);
static uint16_t menu_get_num_rows_callback(MenuLayer *me, uint16_t section_index, void *data);
static int16_t menu_get_header_height_callback(MenuLayer *me, uint16_t section_index, void *data);
static int16_t menu_get_cell_height_callback(MenuLayer *me, MenuIndex *cell_index, void *data);
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static void menu_select_click_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void vibration_tick_callback(TimerVibration vibration);
static void vibration_full_callback(TimerVibration vibration);
static void duration_callback(uint32_t duration);
static void accel_tick_callback(uint8_t amount);
static void energy_amount_callback(uint8_t amount);

static Window *window;
static MenuLayer *layer_menu;

void settings_screen_init(void)
{
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers){
                                         .load = window_load,
                                         .unload = window_unload});
}

void settings_screen_show(void)
{
  window_stack_push(window, true);
}

static void window_load(Window *window)
{
  layer_menu = menu_layer_create_fullscreen(window);
  menu_layer_set_callbacks(layer_menu, NULL, (MenuLayerCallbacks){
                                                 .get_num_sections = menu_get_num_sections_callback,
                                                 .get_num_rows = menu_get_num_rows_callback,
                                                 .get_header_height = menu_get_header_height_callback,
                                                 .get_cell_height = menu_get_cell_height_callback,
                                                 .draw_header = menu_draw_header_callback,
                                                 .draw_row = menu_draw_row_callback,
                                                 .select_click = menu_select_click_callback,
                                             });
  menu_layer_set_click_config_onto_window(layer_menu, window);
  menu_layer_add_to_window(layer_menu, window);
}

static void window_unload(Window *window)
{
  DEBUG("Settings Window Unload");
  menu_layer_destroy_safe(layer_menu);
}

static uint16_t menu_get_num_sections_callback(MenuLayer *me, void *data)
{
  return MENU_NUM_SECTIONS;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *me, uint16_t section_index, void *data)
{
  switch (section_index)
  {
  case MENU_SECTION_CHARACTER:
    return MENU_SECTION_ROWS_CHARACTER;
  case MENU_SECTION_TIMERS:
    return MENU_SECTION_ROWS_TIMERS;
  }
  return 0;
}

static int16_t menu_get_header_height_callback(MenuLayer *me, uint16_t section_index, void *data)
{
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static int16_t menu_get_cell_height_callback(MenuLayer *me, MenuIndex *cell_index, void *data)
{
  //return PBL_IF_ROUND_ELSE(
  //menu_layer_is_index_selected(me, cell_index) ?
  //MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT : MENU_CELL_ROUND_UNFOCUSED_TALL_CELL_HEIGHT,
  //32);
  return 32;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data)
{
  bool invert = false;
  if(menu_cell_layer_is_highlighted(cell_layer)) {
      invert = true;
  }
  char value[16] = "";

  switch (cell_index->section)
  {
  case MENU_SECTION_CHARACTER:
    switch (cell_index->row)
    {
    case MENU_ROW_CHARACTER_ENERGY:
      snprintf(value, 16, "%02d", settings()->max_energy);
      menu_draw_option(ctx, "Max Energy", value, invert);
      break;
    }
    break;
  case MENU_SECTION_TIMERS:
    switch (cell_index->row)
    {
    case MENU_ROW_TIMERS_DURATION:
      timer_time_str(settings()->timers_duration, value, 16);
      menu_draw_option(ctx, "Duration", value, invert);
      break;
    case MENU_ROW_TIMERS_ACCEL_ENABLE:
      menu_draw_option(ctx, "Accelerate", settings()->accel_enabled ? "ON" : "OFF", invert);
      break;
    case MENU_ROW_TIMERS_ACCEL_TICK:
      snprintf(value, 16, "%02d", settings()->accel_tick);
      menu_draw_option(ctx, "Accel Tick", value, invert);
      break;
    case MENU_ROW_TIMERS_VIBRATION_TICK:
      strcpy(value, timer_vibe_str(settings()->timers_tick_vibration, true));
      uppercase(value);
      menu_draw_option(ctx, "Tick Vibe", value, invert);
      break;
    case MENU_ROW_TIMERS_VIBRATION_FULL:
      strcpy(value, timer_vibe_str(settings()->timers_finish_vibration, true));
      uppercase(value);
      menu_draw_option(ctx, "Full Vibe", value, invert);
      break;
    }
    break;
  }
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *data)
{
  switch (section_index)
  {
  case MENU_SECTION_CHARACTER:
    menu_cell_basic_header_draw(ctx, cell_layer, "Character Settings");
    break;
  case MENU_SECTION_TIMERS:
    menu_cell_basic_header_draw(ctx, cell_layer, "Timer Settings");
    break;
  }
}

static void menu_select_click_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context)
{
  switch (cell_index->section)
  {
  case MENU_SECTION_CHARACTER:
    switch (cell_index->row)
    {
    case MENU_ROW_CHARACTER_ENERGY:
      amount_screen_show(settings()->max_energy, "ENERGY", energy_amount_callback);
      break;
    }
    break;
  case MENU_SECTION_TIMERS:
    switch (cell_index->row)
    {
    case MENU_ROW_TIMERS_DURATION:
      duration_screen_show(settings()->timers_duration, duration_callback);
      break;
    case MENU_ROW_TIMERS_ACCEL_ENABLE:
      settings()->accel_enabled = !settings()->accel_enabled;
      break;
    case MENU_ROW_TIMERS_ACCEL_TICK:
      amount_screen_show(settings()->accel_tick, "Tick", accel_tick_callback);
      break;
    case MENU_ROW_TIMERS_VIBRATION_TICK:
      vibration_screen_show(vibration_tick_callback, settings()->timers_tick_vibration);
      break;
    case MENU_ROW_TIMERS_VIBRATION_FULL:
      vibration_screen_show(vibration_full_callback, settings()->timers_finish_vibration);
      break;
    }
    break;
  }
  timers_mark_updated();
}

static void vibration_tick_callback(TimerVibration vibration)
{
  settings()->timers_tick_vibration = vibration;
}

static void vibration_full_callback(TimerVibration vibration)
{
  settings()->timers_finish_vibration = vibration;
}

static void duration_callback(uint32_t duration)
{
  settings()->timers_duration = duration;
}

static void accel_tick_callback(uint8_t amount)
{
  settings()->accel_tick = amount;
}

static void energy_amount_callback(uint8_t amount)
{
  settings()->max_energy = amount;
  settings()->current_energy = amount;
}
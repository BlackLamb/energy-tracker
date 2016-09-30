#include <pebble.h>
#include <@smallstoneapps/utils/macros.h>
#include "timer_add_screen.h"
#include "timers.h"
#include "timer.h"
#include "common.h"
#include "settings.h"
#include "duration_screen.h"
#include "amount_screen.h"

#define MENU_SECTION_MAIN 0
#define MENU_SECTION_FOOTER 1

#define MENU_ROW_DURATION 0
#define MENU_ROW_BASE_TICK 1


static void window_load(Window *window);
static void window_unload(Window *window);
static uint16_t menu_get_num_sections_callback(MenuLayer *me, void *data);
static uint16_t menu_get_num_rows_callback(MenuLayer *me, uint16_t section_index, void *data);
static int16_t menu_get_header_height_callback(MenuLayer *me, uint16_t section_index, void *data);
static int16_t menu_get_cell_height_callback(MenuLayer *me, MenuIndex *cell_index, void *data);
static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void menu_draw_row_main(GContext *ctx, uint16_t row, bool invert);
static void menu_select_click_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_main(uint16_t row);
static void menu_select_footer(void);
static void amount_callback(uint8_t amount);
static void duration_callback(uint32_t duration);

static Window *s_window;
static MenuLayer *s_menu;
static Timer *s_timer = NULL;
static bool s_mode_edit = false;

void timer_add_screen_init(void)
{
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = window_load,
                                           .unload = window_unload});
}

void timer_add_screen_show_new(void)
{
  window_stack_push(s_window, true);
  if (NULL != s_timer)
  {
    free(s_timer);
    s_timer = NULL;
  }
  s_timer = timer_create_timer();
  s_mode_edit = false;
}

void timer_add_screen_show_edit(Timer *tmr)
{
  window_stack_push(s_window, true);
  s_mode_edit = true;
  s_timer = timer_clone(tmr);
}

void timer_add_screen_destroy(void) 
{
  window_destroy_safe(s_window);
  if (NULL != s_timer)
  {
    free(s_timer);
    s_timer = NULL;
  }
}

static void window_load(Window *window)
{
  s_menu = menu_layer_create_fullscreen(s_window);
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
                                             .get_num_sections = menu_get_num_sections_callback,
                                             .get_num_rows = menu_get_num_rows_callback,
                                             .get_header_height = menu_get_header_height_callback,
                                             .get_cell_height = menu_get_cell_height_callback,
                                             .draw_header = menu_draw_header_callback,
                                             .draw_row = menu_draw_row_callback,
                                             .select_click = menu_select_click_callback,
                                         });
  menu_layer_add_to_window(s_menu, s_window);
}

static void window_unload(Window *s_window)
{
  DEBUG("Timer Add Window Unload");
  menu_layer_destroy_safe(s_menu);
}

static uint16_t menu_get_num_sections_callback(MenuLayer *me, void *data)
{
  return 2;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *me, uint16_t section_index, void *data)
{
  switch (section_index)
  {
  case MENU_SECTION_MAIN:
    return 2;
  case MENU_SECTION_FOOTER:
    return 1;
  }
  return 0;
}

static int16_t menu_get_header_height_callback(MenuLayer *me, uint16_t section_index, void *data)
{
  switch (section_index)
  {
  case MENU_SECTION_FOOTER:
    return 4;
    break;
  }
  return 0;
}

static int16_t menu_get_cell_height_callback(MenuLayer *me, MenuIndex *cell_index, void *data)
{
  if (cell_index->section == MENU_SECTION_FOOTER)
  {
    return 44;
  }
  return 32;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *data)
{
  return;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data)
{
  bool invert = false;
  if(menu_cell_layer_is_highlighted(cell_layer)) {
      invert = true;
  }
  switch (cell_index->section)
  {
  case MENU_SECTION_MAIN:
    menu_draw_row_main(ctx, cell_index->row, invert);
    break;
  case MENU_SECTION_FOOTER:
    graphics_context_set_text_color(ctx, GColorBlack);
    if (invert) 
    {
        graphics_context_set_text_color(ctx, GColorWhite);
    }
    graphics_draw_text(ctx, s_mode_edit ? "Save Timer" : "Add Timer",
                       fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(4, 5, 136, 28),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    break;
  }
}

static void menu_draw_row_main(GContext *ctx, uint16_t row, bool invert)
{
  char tmp[16];
  switch (row)
  {
  case MENU_ROW_DURATION:
    timer_time_str(s_timer->length, tmp, sizeof(tmp));
    menu_draw_option(ctx, "Rate", tmp, invert);
    break;
  case MENU_ROW_BASE_TICK:
    snprintf(tmp, 16, "%02d", s_timer->base_amount);
    menu_draw_option(ctx, "Energy", tmp, invert);
    break;
  }
}

static void menu_select_click_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context)
{
  switch (cell_index->section)
  {
  case MENU_SECTION_MAIN:
    menu_select_main(cell_index->row);
    break;
  case MENU_SECTION_FOOTER:
    menu_select_footer();
    break;
  }
}

static void menu_select_main(uint16_t row)
{
  switch (row)
  {
  case MENU_ROW_DURATION:
    duration_screen_show(s_timer->length, duration_callback);
    break;
  case MENU_ROW_BASE_TICK:
    amount_screen_show(s_timer->base_amount, "ENERGY", amount_callback);
    break;
  }
}

static void menu_select_footer(void)
{
  if (s_timer->length == 0)
  {
    vibes_short_pulse();
    menu_layer_set_selected_index(s_menu, (MenuIndex){MENU_SECTION_MAIN, MENU_ROW_DURATION}, MenuRowAlignCenter, true);
    return;
  }
  if (s_mode_edit)
  {
    Timer *timer = timers_find(s_timer->id);
    timer->length = s_timer->length;
    timer->base_amount = s_timer->base_amount;
    timer_reset(timer,false);
    window_stack_pop(true);
    timers_mark_updated();
  }
  else
  {
    Timer *timer = timer_clone(s_timer);
    timer_reset(timer, false);
    timers_add(timer);
    window_stack_pop(true);
    timers_mark_updated();
    timers_highlight(timer);
  }
}

static void amount_callback(uint8_t amount)
{
  s_timer->base_amount = amount;
}

static void duration_callback(uint32_t duration)
{
  s_timer->length = duration;
}

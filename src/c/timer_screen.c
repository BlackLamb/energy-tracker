#include <pebble.h>
#include <@smallstoneapps/utils/macros.h>
#include <@smallstoneapps/bitmap-loader/bitmap-loader.h>

#include "timer_screen.h"
#include "timer_add_screen.h"
#include "timer.h"
#include "settings.h"
#include "common.h"
#include "icons.h"
#include "timers.h"

#define MENU_ROW_PAUSE 0
#define MENU_ROW_RESET 1
#define MENU_ROW_DELETE 2
#define MENU_ROW_EDIT 3

static void window_load(Window *window);
static void window_unload(Window *window);
static void layer_header_update(Layer *layer, GContext *ctx);

static uint16_t menu_num_sections(struct MenuLayer *menu, void *callback_context);
static uint16_t menu_num_rows(struct MenuLayer *menu, uint16_t section_index, void *callback_context);
static int16_t menu_cell_height(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context);
static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context);

static void timers_update_handler(void);
static bool can_edit(void);

static Window *s_window;
static Timer *s_timer;
static Layer *s_layer_header;
static MenuLayer *s_layer_menu;

void timer_screen_init(void)
{
  s_timer = NULL;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = window_load,
                                           .unload = window_unload,
                                       });
  timers_register_update_handler(timers_update_handler);
}

void timer_screen_set_timer(Timer *timer)
{
  s_timer = timer;
}

void timer_screen_show(void)
{
  if (!s_timer)
  {
    return;
  }
  window_stack_push(s_window, false);
}

void timer_screen_destroy(void) 
{
  window_destroy_safe(s_window);
}

static void window_load(Window *window)
{
  s_layer_header = layer_create(GRect(0, 0, PEBBLE_WIDTH, 36));
  layer_set_update_proc(s_layer_header, layer_header_update);
  layer_add_to_window(s_layer_header, s_window);

  s_layer_menu = menu_layer_create(GRect(0, 36, PEBBLE_WIDTH, PEBBLE_HEIGHT - STATUS_HEIGHT - 36));
  menu_layer_set_callbacks(s_layer_menu, NULL, (MenuLayerCallbacks){
                                                   .get_num_sections = menu_num_sections,
                                                   .get_num_rows = menu_num_rows,
                                                   .get_cell_height = menu_cell_height,
                                                   .draw_row = menu_draw_row,
                                                   .select_click = menu_select,
                                               });
  menu_layer_add_to_window(s_layer_menu, s_window);
}

static void window_unload(Window *window)
{
  DEBUG("Timer Window Unload");
  menu_layer_destroy_safe(s_layer_menu);
  layer_destroy_safe(s_layer_header);
}

static void layer_header_update(Layer *layer, GContext *ctx)
{
  timer_draw_row(s_timer, false, true, ctx, false);
}

static uint16_t menu_num_sections(struct MenuLayer *menu, void *callback_context)
{
  return 1;
}

static uint16_t menu_num_rows(struct MenuLayer *menu, uint16_t section_index, void *callback_context)
{
  return can_edit() ? 4 : 3;
}

static int16_t menu_cell_height(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context)
{
  return 32;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context)
{
  bool invert = false;
  if(menu_cell_layer_is_highlighted(cell_layer)) {
      invert = true;
  }
  switch (cell_index->row)
  {
  case MENU_ROW_PAUSE:
  {
    switch (s_timer->status)
    {
    case TIMER_STATUS_RUNNING:
      menu_draw_row_icon_text(ctx, "Pause", bitmaps_get_sub_bitmap(invert ? RESOURCE_ID_ICON_16_INVERTED : RESOURCE_ID_ICONS_16, ICON_RECT_PAUSE), invert);
      break;
    case TIMER_STATUS_PAUSED:
      menu_draw_row_icon_text(ctx, "Resume", bitmaps_get_sub_bitmap(invert ? RESOURCE_ID_ICON_16_INVERTED : RESOURCE_ID_ICONS_16, ICON_RECT_PAUSE), invert);
      break;
    case TIMER_STATUS_DONE:
    case TIMER_STATUS_STOPPED:
      menu_draw_row_icon_text(ctx, "Start", bitmaps_get_sub_bitmap(invert ? RESOURCE_ID_ICON_16_INVERTED : RESOURCE_ID_ICONS_16, ICON_RECT_PLAY), invert);
      break;
    }
    break;
  }
  case MENU_ROW_RESET:
    menu_draw_row_icon_text(ctx, "Reset", bitmaps_get_sub_bitmap(invert ? RESOURCE_ID_ICON_16_INVERTED : RESOURCE_ID_ICONS_16, ICON_RECT_RESET), invert);
    break;
  case MENU_ROW_DELETE:
    menu_draw_row_icon_text(ctx, "Delete", bitmaps_get_sub_bitmap(invert ? RESOURCE_ID_ICON_16_INVERTED : RESOURCE_ID_ICONS_16, ICON_RECT_DELETE), invert);
    break;
  case MENU_ROW_EDIT:
    menu_draw_row_icon_text(ctx, "Edit", bitmaps_get_sub_bitmap(invert ? RESOURCE_ID_ICON_16_INVERTED : RESOURCE_ID_ICONS_16, ICON_RECT_EDIT), invert);
    break;
  }
}

static void menu_select(struct MenuLayer *menu, MenuIndex *cell_index, void *callback_context)
{
  switch (cell_index->row)
  {
  case MENU_ROW_PAUSE:
  {
    switch (s_timer->status)
    {
    case TIMER_STATUS_RUNNING:
      timer_pause(s_timer);
      break;
    case TIMER_STATUS_PAUSED:
      timer_resume(s_timer);
      break;
    case TIMER_STATUS_DONE:
    case TIMER_STATUS_STOPPED:
      timer_start(s_timer);
      break;
    }
    break;
  }
  case MENU_ROW_RESET:
    timer_reset(s_timer, true, false);
    settings()->quicken_enabled = false;
    break;
  case MENU_ROW_DELETE:
    timers_remove(timers_index_of(s_timer->id));
    window_stack_pop(false);
    break;
  case MENU_ROW_EDIT:
    timer_add_screen_show_edit(s_timer);
    break;
  }
}

static void timers_update_handler(void)
{
  if (!window_is_loaded(s_window))
  {
    return;
  }
  layer_mark_dirty(s_layer_header);
  menu_layer_reload_data(s_layer_menu);
}

static bool can_edit(void)
{
  if (s_timer->status == TIMER_STATUS_STOPPED)
  {
    return true;
  }
  return false;
}
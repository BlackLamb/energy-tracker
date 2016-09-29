#include <pebble.h>
#include "main_screen.h"

#include <@smallstoneapps/utils/macros.h>
#include <@smallstoneapps/bitmap-loader/bitmap-loader.h>

#include "timers.h"
#include "timer.h"
#include "settings.h"
#include "icons.h"
#include "menu_screen.h"

#define STATUS_START_HEIGHT 110
#define STATUS_BOX_WIDTH PBL_IF_RECT_ELSE(113, 151)
#define STATUS_BOX_HEIGHT PBL_IF_RECT_ELSE(58, 73)

static Window *s_window;
static GFont s_res_gothic_18_bold;
static ActionBarLayer *s_actionbarlayer;
static StatusBarLayer *s_statusbarlayer;
static TextLayer *s_current_energy_layer;
static TextLayer *s_total_energy_layer;
static BitmapLayer *s_bitmap_status_area_layer;
static TextLayer *s_timer_rate_layer;
static TextLayer *s_timer_next_layer;
static TextLayer *s_timer_full_layer;
static char s_str_current_eng[] = "99";
static char s_str_max_eng[] = "/99";
static Timer *s_current_timer = NULL;
static char s_str_timer_rate[] = "00 / 00:00";
static char s_str_timer_next[] = "Next: 04:30";
static char s_str_timer_full[] = "Full: 29:30";

static void initialise_ui(void);
static void destroy_ui(void);
static void handle_window_unload(Window *window);
static void anim_stopped_handler(Animation *animation, bool finished, void *context);
static void timers_update_handler(void);
static void update_energy(int8_t amount);
static void layer_action_bar_click_config_provider(void *context);
static void action_bar_layer_down_handler(ClickRecognizerRef recognizer, void *context);
static void action_bar_layer_up_handler(ClickRecognizerRef recognizer, void *context);
static void action_bar_layer_select_handler(ClickRecognizerRef recognizer, void *context);
static void action_bar_layer_long_select_handler(ClickRecognizerRef recognizer, void *context);
static void update_timer_status(void);

void main_screen_init(void)
{
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .unload = handle_window_unload,
                                       });
  menu_screen_init();
  timers_register_update_handler(timers_update_handler);
}

void main_screen_show(void)
{
  window_stack_push(s_window, true);
}

void main_screen_show_status_area(Timer *timer)
{
  Window *current_window = window_stack_get_top_window();
  if (window_stack_contains_window(s_window) && current_window != s_window) 
  {
      while (current_window != s_window) 
      {
          window_stack_pop(false);
          current_window = window_stack_get_top_window();
      }
  }
  
  s_current_timer = timer;
  //Timer Text
  update_timer_status();

  //Animation
  layer_show((Layer *)s_bitmap_status_area_layer);
  GRect anim_start = GRect(PBL_IF_RECT_ELSE(-1, -3), PEBBLE_HEIGHT, STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
  GRect anim_end = GRect(PBL_IF_RECT_ELSE(-1, -3), STATUS_START_HEIGHT, STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
  GRect layer_current = layer_get_frame((Layer *)s_bitmap_status_area_layer);

  if (&layer_current == &anim_end)
  {
    return;
  }

  PropertyAnimation *prop_anim = property_animation_create_layer_frame((Layer *)s_bitmap_status_area_layer, &anim_start, &anim_end);
  Animation *anim = property_animation_get_animation(prop_anim);
  animation_set_curve(anim, AnimationCurveEaseOut);
  animation_set_duration(anim, 750);
  animation_schedule(anim);
}

void main_screen_hide_status_area(bool finished)
{
  GRect anim_start = GRect(PBL_IF_RECT_ELSE(-1, -3), STATUS_START_HEIGHT, STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
  GRect anim_end = GRect(PBL_IF_RECT_ELSE(-1, -3), PEBBLE_HEIGHT, STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
  GRect layer_current = layer_get_frame((Layer *)s_bitmap_status_area_layer);

  if (&layer_current == &anim_end)
  {
    return;
  }

  PropertyAnimation *prop_anim = property_animation_create_layer_frame((Layer *)s_bitmap_status_area_layer, &anim_start, &anim_end);
  Animation *anim = property_animation_get_animation(prop_anim);
  animation_set_curve(anim, AnimationCurveEaseOut);
  animation_set_duration(anim, 750);
  animation_set_handlers(anim, (AnimationHandlers){
                                   .stopped = anim_stopped_handler},
                         NULL);
  if (finished)
  {
    animation_set_delay(anim, 5000);
    text_layer_set_text(s_timer_next_layer, "FULL!!!");
    text_layer_set_text(s_timer_full_layer, "");
  }
  animation_schedule(anim);
}

Timer * main_screen_active_timer(void) 
{
    return s_current_timer;
}

static void initialise_ui(void)
{
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  uint16_t status_start = STATUS_START_HEIGHT;
  uint16_t status_width = STATUS_BOX_WIDTH;
  uint16_t status_height = STATUS_BOX_HEIGHT;
  snprintf(s_str_max_eng, sizeof(s_str_max_eng), "/%i", settings()->max_energy);
  snprintf(s_str_current_eng, sizeof(s_str_current_eng), "%i", settings()->current_energy);

  // s_bitmap_status_area_layer
  s_bitmap_status_area_layer = bitmap_layer_create(GRect(PBL_IF_RECT_ELSE(-1, -3), PEBBLE_HEIGHT, status_width, status_height));
  bitmap_layer_set_bitmap(s_bitmap_status_area_layer, bitmaps_get_bitmap(RESOURCE_ID_IMAGE_BOTTOM_STATUS));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_bitmap_status_area_layer);

  // s_actionbarlayer
  s_actionbarlayer = action_bar_layer_create();
  action_bar_layer_add_to_window(s_actionbarlayer, s_window);
  action_bar_layer_set_background_color(s_actionbarlayer, GColorWhite);
  action_bar_layer_set_click_config_provider(s_actionbarlayer, layer_action_bar_click_config_provider);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_UP, bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS_16, ICON_RECT_ACTION_INC));
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_SELECT, bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS_16, ICON_RECT_ACTION_TICK));
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_DOWN, bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS_16, ICON_RECT_ACTION_DEC));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_actionbarlayer);

  // s_statusbarlayer
  s_statusbarlayer = status_bar_layer_create();
  status_bar_layer_set_colors(s_statusbarlayer, GColorClear, GColorWhite);
  status_bar_layer_set_separator_mode(s_statusbarlayer, StatusBarLayerSeparatorModeNone);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_statusbarlayer);

  // s_current_energy_layer
  s_current_energy_layer = text_layer_create(GRect(0, 44, (PEBBLE_WIDTH / 2) - 14, 48));
  text_layer_set_background_color(s_current_energy_layer, GColorClear);
  text_layer_set_text_color(s_current_energy_layer, GColorWhite);
  text_layer_set_text(s_current_energy_layer, s_str_current_eng);
  text_layer_set_text_alignment(s_current_energy_layer, GTextAlignmentRight);
  text_layer_set_font(s_current_energy_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_current_energy_layer);

  // s_total_energy_layer
  s_total_energy_layer = text_layer_create(GRect((PEBBLE_WIDTH / 2) - 18, 64, (PEBBLE_WIDTH / 2) - 15, 34));
  text_layer_set_background_color(s_total_energy_layer, GColorClear);
  text_layer_set_text_color(s_total_energy_layer, GColorWhite);
  text_layer_set_text(s_total_energy_layer, s_str_max_eng);
  text_layer_set_font(s_total_energy_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_total_energy_layer);

  status_width += PBL_IF_RECT_ELSE(0, 15);
  // s_timer_rate_layer
  s_timer_rate_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(1, 3), 0, PBL_IF_RECT_ELSE(status_width - 1, PEBBLE_WIDTH), 18));
  text_layer_set_background_color(s_timer_rate_layer, GColorClear);
  text_layer_set_text_color(s_timer_rate_layer, GColorBlack);
  text_layer_set_text(s_timer_rate_layer, "00 / 00:00");
  text_layer_set_text_alignment(s_timer_rate_layer, GTextAlignmentCenter);
  text_layer_set_font(s_timer_rate_layer, s_res_gothic_18_bold);
  layer_add_child((Layer *)s_bitmap_status_area_layer, (Layer *)s_timer_rate_layer);

  status_start += 18;
  // s_timer_next_layer
  s_timer_next_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(1, 3), 18, PBL_IF_RECT_ELSE(status_width - 1, PEBBLE_WIDTH), 18));
  text_layer_set_background_color(s_timer_next_layer, GColorClear);
  text_layer_set_text_color(s_timer_next_layer, GColorBlack);
  text_layer_set_text(s_timer_next_layer, "Next: 04:30");
  text_layer_set_text_alignment(s_timer_next_layer, GTextAlignmentCenter);
  text_layer_set_font(s_timer_next_layer, s_res_gothic_18_bold);
  layer_add_child((Layer *)s_bitmap_status_area_layer, (Layer *)s_timer_next_layer);

  // s_timer_full_layer
  s_timer_full_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(1, 3), 36, PBL_IF_RECT_ELSE(status_width - 1, PEBBLE_WIDTH), 18));
  text_layer_set_background_color(s_timer_full_layer, GColorClear);
  text_layer_set_text_color(s_timer_full_layer, GColorBlack);
  text_layer_set_text(s_timer_full_layer, "Full: 29:30");
  text_layer_set_text_alignment(s_timer_full_layer, GTextAlignmentCenter);
  text_layer_set_font(s_timer_full_layer, s_res_gothic_18_bold);
  layer_add_child((Layer *)s_bitmap_status_area_layer, (Layer *)s_timer_full_layer);

  layer_hide((Layer *)s_bitmap_status_area_layer);
}

static void destroy_ui(void)
{
  window_destroy_safe(s_window);
  text_layer_destroy_safe(s_timer_rate_layer);
  text_layer_destroy_safe(s_timer_next_layer);
  text_layer_destroy_safe(s_timer_full_layer);
  bitmap_layer_destroy_safe(s_bitmap_status_area_layer);
  action_bar_layer_destroy_safe(s_actionbarlayer);
  status_bar_layer_destroy(s_statusbarlayer);
  text_layer_destroy_safe(s_current_energy_layer);
  text_layer_destroy_safe(s_total_energy_layer);
}

static void handle_window_unload(Window *window)
{
  DEBUG("Main Window Unload");
  destroy_ui();
}

static void anim_stopped_handler(Animation *animation, bool finished, void *context)
{
  layer_hide((Layer *)s_bitmap_status_area_layer);
}

static void timers_update_handler (void)
{
  update_timer_status();
}

static void update_energy(int8_t amount)
{
  settings()->current_energy += amount;
  if (settings()->current_energy > settings()->max_energy)
  {
    settings()->current_energy = settings()->max_energy;
  }
  if (settings()->current_energy < 0)
  {
    settings()->current_energy = 0;
  }
  snprintf(s_str_max_eng, sizeof(s_str_max_eng), "/%i", settings()->max_energy);
  snprintf(s_str_current_eng, sizeof(s_str_current_eng), "%i", settings()->current_energy);
  text_layer_set_text(s_current_energy_layer, s_str_current_eng);
  text_layer_set_text(s_total_energy_layer, s_str_max_eng);
}

static void layer_action_bar_click_config_provider(void *context)
{
  window_single_click_subscribe(BUTTON_ID_UP, action_bar_layer_up_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, action_bar_layer_down_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, action_bar_layer_select_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, action_bar_layer_long_select_handler, NULL);
}

static void action_bar_layer_down_handler(ClickRecognizerRef recognizer, void *context)
{
  if (s_current_timer)
  {
      if (s_current_timer->status != TIMER_STATUS_STOPPED) 
      {
          timer_reset(s_current_timer, true);
      }
  }
  update_energy(-1);
}

static void action_bar_layer_up_handler(ClickRecognizerRef recognizer, void *context)
{
  if (s_current_timer)
  {
      if (s_current_timer->status != TIMER_STATUS_STOPPED) 
      {
          timer_reset(s_current_timer, true);
      }
  }
  update_energy(1);
}

static void action_bar_layer_select_handler(ClickRecognizerRef recognizer, void *context)
{
  menu_screen_show();
}

static void action_bar_layer_long_select_handler(ClickRecognizerRef recognizer, void *context) 
{
    if (s_current_timer) 
    {
        switch (s_current_timer->status) 
        {
          case TIMER_STATUS_RUNNING:
            timer_pause(s_current_timer);
            break;
          case TIMER_STATUS_PAUSED:
            timer_resume(s_current_timer);
            break;
          default:
            break;
        }
    }
}

static void update_timer_status(void)
{
  if (NULL == s_current_timer)
  {
    //main_screen_hide_status_area(false);
    return;
  }
  snprintf(s_str_current_eng, sizeof(s_str_current_eng), "%i", settings()->current_energy);
  int minutes = s_current_timer->length / 60;
  int seconds = s_current_timer->length % 60;
  uint8_t eng_amount = s_current_timer->current_amount;
  snprintf(s_str_timer_rate, sizeof(s_str_timer_rate), "%i / %02d:%02d", eng_amount, minutes, seconds);

  minutes = s_current_timer->current_time / 60;
  seconds = s_current_timer->current_time % 60;
  snprintf(s_str_timer_next, sizeof(s_str_timer_next), "Next: %02d:%02d", minutes, seconds);

  //TODO: Get full time estimate
  minutes = s_current_timer->full_time / 60;
  seconds = s_current_timer->full_time % 60;
  snprintf(s_str_timer_full, sizeof(s_str_timer_full), "Full: %d:%02d", minutes, seconds);

  
  text_layer_set_text(s_current_energy_layer, s_str_current_eng);
  text_layer_set_text(s_timer_rate_layer, s_str_timer_rate);
  text_layer_set_text(s_timer_next_layer, s_str_timer_next);
  text_layer_set_text(s_timer_full_layer, s_str_timer_full);
}
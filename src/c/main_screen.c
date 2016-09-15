#include <pebble.h>
#include "main_screen.h"

#include <@smallstoneapps/utils/macros.h>
#include <@smallstoneapps/bitmap-loader/bitmap-loader.h>

#include "src/c/timers.h"
#include "src/c/settings.h"
#include "src/c/icons.h"

#define STATUS_START_HEIGHT 110
#define STATUS_BOX_WIDTH PBL_IF_RECT_ELSE(113, 151)
#define STATUS_BOX_HEIGHT PBL_IF_RECT_ELSE(58, 73)

static Window *s_window;
static GFont s_res_gothic_18_bold;
static GFont s_res_gothic_28_bold;
static GFont s_res_bitham_42_medium_numbers;
static GFont s_res_bitham_30_black;
static ActionBarLayer *s_actionbarlayer;
static TextLayer *s_timelayer;
static TextLayer *s_current_energy_layer;
static TextLayer *s_total_energy_layer;
static BitmapLayer *s_bitmap_status_area_layer;
static TextLayer *s_timer_rate_layer;
static TextLayer *s_timer_next_layer;
static TextLayer *s_timer_full_layer;

static void initialise_ui(void);
static void destroy_ui(void);
static void handle_window_unload(Window* window);
static void anim_stopped_handler(Animation *animation, bool finished, void *context);
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void timers_update_handler(void);

void main_screen_init(void) {
	initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
}

void main_screen_show(void) {
  window_stack_push(s_window, true);
}

void main_screen_show_status_area(void) {
	layer_show((Layer *)s_bitmap_status_area_layer);
	GRect anim_start = GRect(PBL_IF_RECT_ELSE(-1, -3), PEBBLE_HEIGHT, STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
	GRect anim_end = GRect(PBL_IF_RECT_ELSE(-1, -3), STATUS_START_HEIGHT, STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
	GRect layer_current = layer_get_frame((Layer *)s_bitmap_status_area_layer);
	
	if (&layer_current == &anim_end) { return; }
	
	PropertyAnimation *prop_anim = property_animation_create_layer_frame((Layer *)s_bitmap_status_area_layer, &anim_start, &anim_end);
	Animation *anim = property_animation_get_animation(prop_anim);
	animation_set_curve(anim, AnimationCurveEaseOut);
	animation_set_duration(anim, 750);
	animation_schedule(anim);
}

void main_screen_hide_status_area(void) {
	GRect anim_start = GRect(PBL_IF_RECT_ELSE(-1, -3), STATUS_START_HEIGHT, STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
	GRect anim_end = GRect(PBL_IF_RECT_ELSE(-1, -3), PEBBLE_HEIGHT, STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
	GRect layer_current = layer_get_frame((Layer *)s_bitmap_status_area_layer);
	
	if (&layer_current == &anim_end) { return; }
	
	PropertyAnimation *prop_anim = property_animation_create_layer_frame((Layer *)s_bitmap_status_area_layer, &anim_start, &anim_end);
	Animation *anim = property_animation_get_animation(prop_anim);
	animation_set_curve(anim, AnimationCurveEaseOut);
	animation_set_duration(anim, 750);
	animation_set_handlers(anim, (AnimationHandlers) {
  		.stopped = anim_stopped_handler
		}, NULL);
	animation_schedule(anim);
}

static void initialise_ui(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  s_res_gothic_18_bold = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_res_gothic_28_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_res_bitham_42_medium_numbers = fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);
  s_res_bitham_30_black = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  uint16_t status_start = STATUS_START_HEIGHT;
  uint16_t status_width = STATUS_BOX_WIDTH;
  uint16_t status_height = STATUS_BOX_HEIGHT;
  char time_str[10];
  clock_copy_time_string(time_str, 10);
	

  // s_bitmap_status_area_layer
  s_bitmap_status_area_layer = bitmap_layer_create(GRect(PBL_IF_RECT_ELSE(-1, -3), PEBBLE_HEIGHT, status_width, status_height));
  bitmap_layer_set_bitmap(s_bitmap_status_area_layer, bitmaps_get_bitmap(RESOURCE_ID_IMAGE_BOTTOM_STATUS));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_bitmap_status_area_layer);

  // s_actionbarlayer
  s_actionbarlayer = action_bar_layer_create();
  action_bar_layer_add_to_window(s_actionbarlayer, s_window);
  action_bar_layer_set_background_color(s_actionbarlayer, GColorWhite);
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_UP, bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS_16, ICON_RECT_ADD));
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_SELECT, bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS_16, ICON_RECT_SETTINGS));
  action_bar_layer_set_icon(s_actionbarlayer, BUTTON_ID_DOWN, bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS_16, ICON_RECT_DELETE));
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_actionbarlayer);
  
  // s_timelayer
  s_timelayer = text_layer_create(GRect(0, 3, PEBBLE_WIDTH, 30));
  text_layer_set_background_color(s_timelayer, GColorClear);
  text_layer_set_text_color(s_timelayer, GColorWhite);
  text_layer_set_text(s_timelayer, time_str);
  text_layer_set_text_alignment(s_timelayer, GTextAlignmentCenter);
  text_layer_set_font(s_timelayer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_timelayer);
  
  // s_current_energy_layer
  s_current_energy_layer = text_layer_create(GRect(0, 44, (PEBBLE_WIDTH / 2) - 14, 48));
  text_layer_set_background_color(s_current_energy_layer, GColorClear);
  text_layer_set_text_color(s_current_energy_layer, GColorWhite);
  text_layer_set_text(s_current_energy_layer, "99");
  text_layer_set_text_alignment(s_current_energy_layer, GTextAlignmentRight);
  text_layer_set_font(s_current_energy_layer, s_res_bitham_42_medium_numbers);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_current_energy_layer);
  
  // s_total_energy_layer
  s_total_energy_layer = text_layer_create(GRect((PEBBLE_WIDTH / 2) - 18, 64, (PEBBLE_WIDTH / 2) - 15, 34));
  text_layer_set_background_color(s_total_energy_layer, GColorClear);
  text_layer_set_text_color(s_total_energy_layer, GColorWhite);
  text_layer_set_text(s_total_energy_layer, "/99");
  text_layer_set_font(s_total_energy_layer, s_res_bitham_30_black);
  layer_add_child(window_get_root_layer(s_window), (Layer *)s_total_energy_layer);
  
  status_width += PBL_IF_RECT_ELSE(0, 15);
  // s_timer_rate_layer
  s_timer_rate_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(1, 3), 0, PBL_IF_RECT_ELSE(status_width - 1, PEBBLE_WIDTH), 18));
  text_layer_set_background_color(s_timer_rate_layer, GColorClear);
  text_layer_set_text_color(s_timer_rate_layer, GColorBlack);
  text_layer_set_text(s_timer_rate_layer, "1 / 5:00");
  text_layer_set_text_alignment(s_timer_rate_layer, GTextAlignmentCenter);
  text_layer_set_font(s_timer_rate_layer, s_res_gothic_18_bold);
  layer_add_child((Layer *)s_bitmap_status_area_layer, (Layer *)s_timer_rate_layer);
  
  status_start += 18;
  // s_timer_next_layer
  s_timer_next_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(1, 3), 18, PBL_IF_RECT_ELSE(status_width - 1, PEBBLE_WIDTH), 18));
  text_layer_set_background_color(s_timer_next_layer, GColorClear);
  text_layer_set_text_color(s_timer_next_layer, GColorBlack);
  text_layer_set_text(s_timer_next_layer, "Next: 4:30");
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

static void destroy_ui(void) {
  window_destroy(s_window);
  bitmap_layer_destroy(s_bitmap_status_area_layer);
  action_bar_layer_destroy(s_actionbarlayer);
  text_layer_destroy(s_timelayer);
  text_layer_destroy(s_current_energy_layer);
  text_layer_destroy(s_total_energy_layer);
  text_layer_destroy(s_timer_rate_layer);
  text_layer_destroy(s_timer_next_layer);
  text_layer_destroy(s_timer_full_layer);
}

static void handle_window_unload(Window* window) {
  destroy_ui();
}

static void anim_stopped_handler(Animation *animation, bool finished, void *context) {
  layer_hide((Layer *)s_bitmap_status_area_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	
}

static void timers_update_handler(void) {
	
}
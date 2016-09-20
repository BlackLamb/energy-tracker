#include <pebble.h>
#include <@smallstoneapps/utils/macros.h>
#include <@smallstoneapps/bitmap-loader/bitmap-loader.h>

#include "amount_screen.h"

#include "icons.h"

static void window_load(Window* window);
static void window_unload(Window* window);
static void layer_update(Layer* me, GContext* ctx);
static void layer_action_bar_click_config_provider(void *context);
static void action_bar_layer_down_handler(ClickRecognizerRef recognizer, void *context);
static void action_bar_layer_up_handler(ClickRecognizerRef recognizer, void *context);
static void action_bar_layer_select_handler(ClickRecognizerRef recognizer, void *context);

static Window* s_window;
static Layer* s_layer;
static ActionBarLayer* s_action_bar;
static uint8_t s_amount = 0;
static char* s_display_str;
static DurationCallback s_callback;

static GFont s_font_duration;

void amount_screen_init(void) {
	s_window = window_create();
	window_set_window_handlers(s_window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload
	});
}

void amount_screen_show(uint8_t amount, char* display_str, AmountCallback callback) {
	s_amount = amount;
	s_callback = callback;
	s_display_str = display_str;
	window_stack_push(s_window, true);
	layer_mark_dirty(s_layer);
}

static void window_load(Window* window) {
	s_layer = layer_create_fullscreen(s_window);
	layer_set_update_proc(s_layer, layer_update);
	layer_add_to_window(s_layer, s_window);
	
	s_action_bar = action_bar_layer_create();
	action_bar_layer_add_to_window(s_action_bar, s_window);
	action_bar_layer_set_click_config_provider(s_action_bar, layer_action_bar_click_config_provider);
	action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS_16, ICON_RECT_ACTION_INC));
	action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS_16, ICON_RECT_ACTION_DEC));
	action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS_16, ICON_RECT_ACTION_TICK));
	
	s_font_duration = fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);
}

static void window_unload(Window* window) {
	action_bar_layer_destroy(s_action_bar);
	layer_destroy(s_layer);
	//fonts_unload_custom_font(s_font_duration);
}

static void layer_update(Layer* me, GContext* ctx) {
	graphics_context_set_text_color(ctx, GColorBlack);
	graphics_context_set_stroke_color(ctx, GColorBlack);

	graphics_draw_text(ctx, s_display_str, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GRect(0, 0, PEBBLE_WIDTH - 16, 28), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
	
	char* num_str = "000";
	snprintf(num_str, 3, "%02d", s_amount);

	graphics_draw_text(ctx, time_str, s_font_duration, GRect(0, 27, PEBBLE_WIDTH - 16, 70), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void layer_action_bar_click_config_provider(void *context) {
	window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, action_bar_layer_down_handler);
	window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, action_bar_layer_up_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, action_bar_layer_select_handler);
}

static void action_bar_layer_down_handler(ClickRecognizerRef recognizer, void *context) {
	s_amount -= 1;
	if (s_amount < 0) {
		s_amount = 99;
	}
	layer_mark_dirty(s_layer);
}

static void action_bar_layer_up_handler(ClickRecognizerRef recognizer, void *context) {
	s_amount += 1;
	if (s_amount >= 100) {
		s_amount = 0;
	}
	layer_mark_dirty(s_layer);
}

static void action_bar_layer_select_handler(ClickRecognizerRef recognizer, void *context) {
	switch (s_mode) {
		s_callback(s_amount);
		window_stack_pop(true);
}
#include <pebble.h>
#include <@smallstoneapps/utils/macros.h>
#include <@smallstoneapps/bitmap-loader/bitmap-loader.h>

#include "duration_screen.h"

#include "timer.h"
#include "common.h"
#include "icons.h"

#define MODE_MINUTES 0
#define MODE_SECONDS 1

static void window_load(Window *window);
static void window_unload(Window *window);
static void layer_update(Layer *me, GContext *ctx);
static void layer_action_bar_click_config_provider(void *context);
static void action_bar_layer_down_handler(ClickRecognizerRef recognizer, void *context);
static void action_bar_layer_up_handler(ClickRecognizerRef recognizer, void *context);
static void action_bar_layer_select_handler(ClickRecognizerRef recognizer, void *context);
static void update_timer_length(void);

static Window *s_window;
static Layer *s_layer;
static ActionBarLayer *s_action_bar;
static uint32_t s_duration = 0;
static DurationCallback s_callback;

static int s_mode = MODE_MINUTES;
static int s_minutes = 0;
static int s_seconds = 0;

static GFont s_font_duration;

void duration_screen_init(void)
{
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
					     .load = window_load,
					     .unload = window_unload});
}

void duration_screen_show(uint16_t duration, DurationCallback callback)
{
    s_mode = MODE_MINUTES;
    s_duration = duration;
    s_callback = callback;
    window_stack_push(s_window, true);
    s_minutes = s_duration / 60;
    s_seconds = s_duration % 60;
    layer_mark_dirty(s_layer);
}

void duration_screen_destroy(void) 
{
  window_destroy_safe(s_window);
}

static void window_load(Window *window)
{
    s_layer = layer_create_fullscreen(s_window);
    layer_set_update_proc(s_layer, layer_update);
    layer_add_to_window(s_layer, s_window);

    s_action_bar = action_bar_layer_create();
    action_bar_layer_add_to_window(s_action_bar, s_window);
    action_bar_layer_set_click_config_provider(s_action_bar, layer_action_bar_click_config_provider);
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, bitmaps_get_sub_bitmap(RESOURCE_ID_ICON_16_INVERTED, ICON_RECT_ACTION_INC));
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, bitmaps_get_sub_bitmap(RESOURCE_ID_ICON_16_INVERTED, ICON_RECT_ACTION_DEC));
    action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, bitmaps_get_sub_bitmap(RESOURCE_ID_ICON_16_INVERTED, ICON_RECT_ACTION_TICK));

    s_font_duration = fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS);
}

static void window_unload(Window *window)
{
    DEBUG("Duration Window Unload");
    action_bar_layer_destroy_safe(s_action_bar);
    layer_destroy_safe(s_layer);
    //fonts_unload_custom_font(s_font_duration);
}

static void layer_update(Layer *me, GContext *ctx)
{
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorBlack);

    char summary_str[32];
    timer_time_str(s_duration, summary_str, 32);
    graphics_draw_text(ctx, summary_str, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), GRect(0, 0, PEBBLE_WIDTH - 16, 28), GTextOverflowModeFill, GTextAlignmentCenter, NULL);

    char *time_str = "000";
    char mode_str[16];
    switch (s_mode)
    {
    case MODE_MINUTES:
	snprintf(time_str, 3, "%02d", s_minutes);
	snprintf(mode_str, 16, s_minutes == 1 ? "MINUTE" : "MINUTES");
	break;
    case MODE_SECONDS:
	snprintf(time_str, 3, "%02d", s_seconds);
	snprintf(mode_str, 16, s_seconds == 1 ? "SECOND" : "SECONDS");
	break;
    }
    graphics_draw_text(ctx, time_str, s_font_duration, GRect(0, 27, PEBBLE_WIDTH - 16, 70), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    graphics_draw_text(ctx, mode_str, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(0, 98, PEBBLE_WIDTH - 16, 18), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void layer_action_bar_click_config_provider(void *context)
{
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, action_bar_layer_down_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, action_bar_layer_up_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, action_bar_layer_select_handler);
}

static void action_bar_layer_down_handler(ClickRecognizerRef recognizer, void *context)
{
    switch (s_mode)
    {
    case MODE_MINUTES:
	s_minutes -= 1;
	if (s_minutes < 0)
	{
	    s_minutes = 99;
	}
	break;
    case MODE_SECONDS:
	s_seconds -= 1;
	if (s_seconds < 0)
	{
	    s_seconds = 60;
	}
	break;
    }
    update_timer_length();
    layer_mark_dirty(s_layer);
}

static void action_bar_layer_up_handler(ClickRecognizerRef recognizer, void *context)
{
    switch (s_mode)
    {
    case MODE_MINUTES:
	s_minutes += 1;
	if (s_minutes >= 100)
	{
	    s_minutes = 99;
	}
	break;
    case MODE_SECONDS:
	s_seconds += 1;
	if (s_seconds >= 60)
	{
	    s_seconds -= 60;
	}
	break;
    }
    update_timer_length();
    layer_mark_dirty(s_layer);
}

static void action_bar_layer_select_handler(ClickRecognizerRef recognizer, void *context)
{
    switch (s_mode)
    {
    case MODE_MINUTES:
	s_mode = MODE_SECONDS;
	layer_mark_dirty(s_layer);
	break;
    case MODE_SECONDS:
	s_callback(s_duration);
	window_stack_pop(true);
	break;
    }
}

static void update_timer_length(void)
{
    s_duration = s_minutes * 60 + s_seconds;
}
#include <pebble.h>
#include <@smallstoneapps/utils/macros.h>
#include "about_screen.h"

static void window_load(Window *window);
static void window_unload(Window *window);
static void layer_header_update(Layer *layer, GContext *ctx);

static Window *s_window;
static Layer *s_layer_header;
static TextLayer *s_text_layer;

void about_screen_init(void)
{
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
					     .load = window_load,
					     .unload = window_unload,
					 });
}

void about_screen_show(void)
{
    window_stack_push(s_window, true);
}

void about_screen_destroy(void) 
{
  window_destroy_safe(s_window);
}

static void window_load(Window *window)
{
    s_layer_header = layer_create(GRect(0, 0, PEBBLE_WIDTH, PBL_IF_RECT_ELSE(24, 40)));
    layer_set_update_proc(s_layer_header, layer_header_update);
    layer_add_to_window(s_layer_header, s_window);

    s_text_layer = text_layer_create(GRect(0, PBL_IF_RECT_ELSE(24, 40), PEBBLE_WIDTH, PEBBLE_HEIGHT - PBL_IF_RECT_ELSE(24, 40)));
    text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text(s_text_layer, "By Brent Lamb.\n\nDesigned for Dark Prospects\nhttps://darkprospects.us/\nbut flexable enough for other games.");
    text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
    text_layer_add_to_window(s_text_layer, s_window);
}

static void window_unload(Window *window)
{
    DEBUG("About Window Unload");
    layer_destroy_safe(s_layer_header);
    text_layer_destroy_safe(s_text_layer);
}

static void layer_header_update(Layer *layer, GContext *ctx)
{
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
    graphics_draw_text(ctx, "Energy Counter", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), PBL_IF_RECT_ELSE(GRect(0, -5, PEBBLE_WIDTH, 24), GRect(0, 15, PEBBLE_WIDTH, 40)), GTextOverflowModeFill, GTextAlignmentCenter, 0);
}
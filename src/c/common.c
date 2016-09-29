#include <pebble.h>
#include <@smallstoneapps/utils/macros.h>
#include <@smallstoneapps/bitmap-loader/bitmap-loader.h>
#include "timer.h"
#include "settings.h"
#include "icons.h"

void menu_draw_row_icon_text(GContext *ctx, char *text, GBitmap *icon, bool invert)
{
    graphics_context_set_text_color(ctx, invert ? GColorWhite : GColorBlack);
    if (icon)
    {
        graphics_draw_bitmap_in_rect(ctx, icon, GRect(8, 8, 16, 16));
    }
    graphics_draw_text(ctx, text,
                       fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       GRect(33, -1, PEBBLE_WIDTH - 33, 24), GTextOverflowModeFill,
                       GTextAlignmentLeft, NULL);
}

void timer_draw_row(Timer *timer, bool rate, bool center, GContext *ctx, bool invert)
{
    char *time_left = malloc(12);
    
	  if (rate) 
		{
			  char *tmp_str = malloc(6);
			  timer_time_str(timer->current_time, tmp_str, 6);
			  snprintf(time_left, 12, "%02d in %s", timer->current_amount, tmp_str);
			  free(tmp_str);
		} 
	  else 
		{
			timer_time_str(timer->current_time, time_left, 12);
		}

	  graphics_context_set_text_color(ctx, invert ? GColorWhite : GColorBlack);
    graphics_context_set_fill_color(ctx, invert ? GColorWhite : GColorBlack);


    graphics_draw_text(ctx, time_left,
                       fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                       center ? GRect(0, -3, PEBBLE_WIDTH, 28) : GRect(33, -3, PEBBLE_WIDTH - 33, 28), GTextOverflowModeFill,
                       center ? GTextAlignmentCenter : GTextAlignmentLeft, NULL);

    GBitmap *bmp_icon = NULL;

    switch (timer->status)
    {
    case TIMER_STATUS_STOPPED:
        bmp_icon = bitmaps_get_sub_bitmap(invert ? RESOURCE_ID_ICON_16_INVERTED : RESOURCE_ID_ICONS_16, ICON_RECT_STOP);
        break;
    case TIMER_STATUS_RUNNING:
        bmp_icon = bitmaps_get_sub_bitmap(invert ? RESOURCE_ID_ICON_16_INVERTED : RESOURCE_ID_ICONS_16, ICON_RECT_PLAY);
        break;
    case TIMER_STATUS_PAUSED:
        bmp_icon = bitmaps_get_sub_bitmap(invert ? RESOURCE_ID_ICON_16_INVERTED : RESOURCE_ID_ICONS_16, ICON_RECT_PAUSE);
        break;
    case TIMER_STATUS_DONE:
        bmp_icon = bitmaps_get_sub_bitmap(invert ? RESOURCE_ID_ICON_16_INVERTED : RESOURCE_ID_ICONS_16, ICON_RECT_DONE);
        break;
    }

    if (bmp_icon)
    {
        graphics_draw_bitmap_in_rect(ctx, bmp_icon, GRect(8, 8, 16, 16));
    }

    uint8_t width = (PEBBLE_WIDTH * timer->current_time) / timer->length;
    graphics_fill_rect(ctx, GRect(0, 31, width, 2), 0, GCornerNone);

    free(time_left);
}

void menu_draw_option(GContext *ctx, char *option, char *value, bool invert)
{
    graphics_context_set_text_color(ctx, invert ? GColorWhite : GColorBlack);
    graphics_draw_text(ctx, option, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(4, 0, 136, 28), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, value, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(4, 5, 136, 20), GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
}

void uppercase(char *str)
{
    char *point = str;
    while (*point != '\0')
    {
        if (*point >= 97 && *point <= 122)
        {
            *point -= 32;
        }
        point += 1;
    }
}
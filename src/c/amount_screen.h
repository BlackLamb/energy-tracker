#pragma once
#include "timer.h"

typedef void (*AmountCallback)(uint8_t amount);

void amount_screen_init(void);
void amount_screen_show(uint8_t amount, char *display_str, AmountCallback callback);
void amount_screen_destroy(void);
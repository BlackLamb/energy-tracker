#include <pebble.h>
#include "timer.h"

#define TIMER_BLOCK_SIZE 4

#define TIMERS_VERSION_CURRENT 1
#define TIMERS_VERSION_TINY 1

typedef void (*TimersUpdatedHandler)(void);
typedef void (*TimerHighlightHandler)(Timer* timer);

void timers_init(void);
uint8_t timers_count(void);
Timer* timers_get(uint8_t index);
Timer* timers_find(uint16_t id);
int16_t timers_index_of(uint16_t id);
bool timers_add(Timer* timer);
bool timers_remove(uint8_t position);
Timer* timers_find_last_wakeup(void);
Timer* timers_find_wakeup_collision(Timer* timer);

void timers_clear(void);
void timers_mark_updated(void);
void timers_highlight(Timer* timer);
void timers_register_update_handler(TimersUpdatedHandler handler);
void timers_register_highlight_handler(TimerHighlightHandler handler);

void timers_save(void);
void timers_restore(void);
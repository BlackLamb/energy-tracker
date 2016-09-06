#include <pebble.h>
#include <@smallstoneapps/linked-list/linked-list.h>
#include "timers.h"
#include "timer.h"

typedef struct {
    Timer timers[TIMER_BLOCK_SIZE];
    uint8_t total_timers;
    time_t save_time;
} TimerBlock;

static void timers_cleanup(void);

LinkedRoot* timers = NULL;
LinkedRoot* update_handlers = NULL;
LinkedRoot* highlight_handlers = NULL;

void timers_init(void) {
    timers_cleanup();
    timers = linked_list_create_root();
    update_handlers = linked_list_create_root();
    highlight_handlers = linked_list_create_root();
}

uint8_t timers_count(void) {
    return linked_list_count(timers);
}

Timer* timers_get(uint8_t index) {
    if (! timers) {
        return NULL;
    }
    return linked_list_get(timers, index);
}

Timer* timers_find(uint16_t id) { 
    uint8_t count = timers_count();
    for (uint8_t c = 0; c < count; c += 1) {
        Timer* timer = timers_get(c);
        if (timer->id == id) {
            return timer;
        }
    }
    return NULL;
}

bool timers_add(Timer* timer) {
    linked_list_append(timers, timer);
    return true;
}

bool timers_remove(uint8_t position) {
    Timer* timer = timers_get(position);
    if (NULL == timer) {
        return false;
    }
    timer_pause(timer);
    linked_list_remove(timers, position);
    free(timer);
    timers_mark_updated();
    return true;
}

Timer* timers_find_last_wakeup(void) {
    Timer* last = NULL;
    uint16_t last_wakeup_time = 0;
    uint8_t count = timers_count();
    for (uint8_t c = 0; c < count; c += 1) {
        Timer* timer = timers_get(c);
        if (timer->wakeup_id < 0) {
            continue;
        }
        if (timer->current_time > last_wakeup_time) {
            last = timer;
            last_wakeup_time = timer->current_time;
        }
    }
    return last;
}

Timer* timers_find_wakeup_collision(Timer* timer) {
    time_t wakeup_time;
    wakeup_query(timer->wakeup_id, &wakeup_time);
    uint8_t count = timers_count();
    for (uint8_t c = 0; c < count; c += 1) {
        Timer* timer_to_check = timers_get(c);
        if (timer_to_check->wakeup_id < 0) {
            continue;
        }
        if (timer_to_check->id == timer->id) {
            continue;
        }
        time_t check_time;
        wakeup_query(timer_to_check->wakeup_id, &check_time);
        if (abs(check_time - wakeup_time) <= 60) {
            return timer_to_check;
        }
    }
    return NULL;
}

void timers_clear(void) {
    if (! timers) {
        return;
    }
    while (linked_list_count(timers) > 0) {
        Timer* timer = (Timer*) linked_list_get(timers, 0);
        linked_list_remove(timers, 0);
        free(timer);
    }
}

void timers_mark_updated(void) {
    uint8_t handler_count = linked_list_count(update_handlers);
    for (uint8_t h = 0; h < handler_count; h += 1) {
        TimersUpdatedHandler handler = linked_list_get(update_handlers, h);
        handler();
    }
}

void timers_highlight(Timer* timer) {
    uint8_t handler_count = linked_list_count(highlight_handlers);
    for (uint8_t h = 0; h < handler_count; h += 1) {
        TimerHighlightHandler handler = linked_list_get(highlight_handlers, h);
        handler(timer);
    }
}

void timers_register_update_handler(TimersUpdatedHandler handler) {
    linked_list_append(update_handlers, handler);
}

void timers_register_highlight_handler(TimerHighlightHandler handler) {
    linked_list_append(highlight_handlers, handler);
}

static void timers_cleanup(void) {
    timers_clear();
    free(timers);
    timers = NULL;
}

void timers_save(void) {

}

void timers_restore(void) {

}


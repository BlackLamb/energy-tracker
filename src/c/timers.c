#include <pebble.h>
#include <@smallstoneapps/linked-list/linked-list.h>
#include "timers.h"
#include "timer.h"
#include "persist.h"

typedef struct
{
    Timer timers[TIMER_BLOCK_SIZE];
    uint8_t total_timers;
    time_t save_time;
} TimerBlock;

static void timers_cleanup(void);

LinkedRoot *timers = NULL;
LinkedRoot *update_handlers = NULL;
LinkedRoot *highlight_handlers = NULL;

void timers_init(void)
{
    timers_cleanup();
    timers = linked_list_create_root();
    update_handlers = linked_list_create_root();
    highlight_handlers = linked_list_create_root();
}

uint8_t timers_count(void)
{
    return linked_list_count(timers);
}

Timer *timers_get(uint8_t index)
{
    if (!timers)
    {
        return NULL;
    }
    return linked_list_get(timers, index);
}

Timer *timers_find(uint16_t id)
{
    uint8_t count = timers_count();
    for (uint8_t c = 0; c < count; c += 1)
    {
        Timer *timer = timers_get(c);
        if (timer->id == id)
        {
            return timer;
        }
    }
    return NULL;
}

int16_t timers_index_of(uint16_t id)
{
    uint8_t count = timers_count();
    for (uint8_t c = 0; c < count; c += 1)
    {
        Timer *timer = timers_get(c);
        if (timer->id == id)
        {
            return c;
        }
    }
    return -1;
}

bool timers_add(Timer *timer)
{
    linked_list_append(timers, timer);
    return true;
}

bool timers_remove(uint8_t position)
{
    Timer *timer = timers_get(position);
    if (NULL == timer)
    {
        return false;
    }
    timer_pause(timer);
    linked_list_remove(timers, position);
    free(timer);
    timers_mark_updated();
    return true;
}

Timer *timers_find_last_wakeup(void)
{
    Timer *last = NULL;
    uint16_t last_wakeup_time = 0;
    uint8_t count = timers_count();
    for (uint8_t c = 0; c < count; c += 1)
    {
        Timer *timer = timers_get(c);
        if (timer->wakeup_id < 0)
        {
            continue;
        }
        if (timer->current_time > last_wakeup_time)
        {
            last = timer;
            last_wakeup_time = timer->current_time;
        }
    }
    return last;
}

Timer *timers_find_wakeup_collision(Timer *timer)
{
    time_t wakeup_time;
    wakeup_query(timer->wakeup_id, &wakeup_time);
    uint8_t count = timers_count();
    for (uint8_t c = 0; c < count; c += 1)
    {
        Timer *timer_to_check = timers_get(c);
        if (timer_to_check->wakeup_id < 0)
        {
            continue;
        }
        if (timer_to_check->id == timer->id)
        {
            continue;
        }
        time_t check_time;
        wakeup_query(timer_to_check->wakeup_id, &check_time);
        if (abs(check_time - wakeup_time) <= 60)
        {
            return timer_to_check;
        }
    }
    return NULL;
}

void timers_clear(void)
{
    if (!timers)
    {
        return;
    }
    while (linked_list_count(timers) > 0)
    {
        Timer *timer = (Timer *)linked_list_get(timers, 0);
        linked_list_remove(timers, 0);
        free(timer);
    }
}

void timers_mark_updated(void)
{
    uint8_t handler_count = linked_list_count(update_handlers);
    for (uint8_t h = 0; h < handler_count; h += 1)
    {
        TimersUpdatedHandler handler = linked_list_get(update_handlers, h);
        handler();
    }
}

void timers_highlight(Timer *timer)
{
    uint8_t handler_count = linked_list_count(highlight_handlers);
    for (uint8_t h = 0; h < handler_count; h += 1)
    {
        TimerHighlightHandler handler = linked_list_get(highlight_handlers, h);
        handler(timer);
    }
}

void timers_register_update_handler(TimersUpdatedHandler handler)
{
    linked_list_append(update_handlers, handler);
}

void timers_register_highlight_handler(TimerHighlightHandler handler)
{
    linked_list_append(highlight_handlers, handler);
}

static void timers_cleanup(void)
{
    timers_clear();
    free(timers);
    timers = NULL;
}

void timers_save(void)
{
    if (timers_count() == 0)
    {
        persist_delete(PERSIST_TIMER_START);
        return;
    }
    TimerBlock *block = NULL;
    uint8_t block_count = 0;
    for (uint8_t b = 0; b < timers_count(); b += 1)
    {
        if (NULL == block)
        {
            block = malloc(sizeof(TimerBlock));
            block->total_timers = timers_count();
            block->save_time = time(NULL);
        }

        uint8_t timer_block_pos = b % TIMER_BLOCK_SIZE;
        block->timers[timer_block_pos] = *timers_get(b);

        bool is_last_timer_in_block = timer_block_pos == (TIMER_BLOCK_SIZE - 1);
        if (is_last_timer_in_block)
        {
            persist_write_data(PERSIST_TIMER_START + block_count, block, sizeof(TimerBlock));
            block_count += 1;
            free(block);
            block = NULL;
        }
    }
    if (block)
    {
        persist_write_data(PERSIST_TIMER_START + block_count, block, sizeof(TimerBlock));
    }
    persist_write_int(PERSIST_TIMERS_VERSION, TIMERS_VERSION_CURRENT);
}

void timers_restore(void)
{
    timers_clear();

    time_t now = time(NULL);
    uint16_t seconds_elapsed = 0;

    TimerBlock *block = NULL;
    if (persist_exists(PERSIST_TIMER_START))
    {
        block = malloc(sizeof(TimerBlock));
        persist_read_data(PERSIST_TIMER_START, block, sizeof(TimerBlock));
        uint8_t num_timers = block->total_timers;
        uint8_t block_offset = 0;
        seconds_elapsed = now - block->save_time;

        for (uint8_t t = 0; t < num_timers; t += 1)
        {
            if (!block)
            {
                block = malloc(sizeof(TimerBlock));
                persist_read_data(PERSIST_TIMER_START + block_offset, block, sizeof(TimerBlock));
            }
            Timer *timer = timer_clone(&block->timers[t % TIMER_BLOCK_SIZE]);
            timers_add(timer);
            timer_restore(timer, seconds_elapsed);
            if (t % TIMER_BLOCK_SIZE == (TIMER_BLOCK_SIZE - 1))
            {
                free(block);
                block = NULL;
                block_offset += 1;
            }
        }
        if (block)
        {
            free(block);
            block = NULL;
        }
    }
}

void timers_destroy(void) {
  timers_cleanup();
  linked_list_clear(timers);
  linked_list_clear(update_handlers);
  linked_list_clear(highlight_handlers);
  
  free(timers);
  free(update_handlers);
  free(highlight_handlers);
}

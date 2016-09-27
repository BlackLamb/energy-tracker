#include <pebble.h>
#include "timer.h"
#include "timers.h"
#include "settings.h"
#include "main_screen.h"

static void timer_tick(void *context);
static void timer_finish(Timer *timer);
static void timer_schedule_tick(Timer *timer);
static void timer_cancel_tick(Timer *timer);
static void timer_schedule_wakeup(Timer *timer, uint16_t offset);
static void timer_cancel_wakeup(Timer *timer);
static void timer_set_id(Timer *timer);
static void timer_completed_action(Timer *timer);

void timer_time_str(uint32_t timer_time, char *str, int str_len)
{
    int minutes = timer_time / 60;
    int seconds = timer_time % 60;
    snprintf(str, str_len, "%02d:%02d", minutes, seconds);
}

void timer_start(Timer *timer)
{
    timer->current_time = timer->length;
    timer->status = TIMER_STATUS_RUNNING;
    timer_schedule_tick(timer);
    timer_schedule_wakeup(timer, 0);
    timer_refresh_info(timer);
    timers_mark_updated();
    main_screen_show_status_area(timer);
}

void timer_pause(Timer *timer)
{
    timer->status = TIMER_STATUS_PAUSED;
    timer_cancel_tick(timer);
    timer_cancel_wakeup(timer);
    timers_mark_updated();
}

void timer_resume(Timer *timer)
{
    timer->status = TIMER_STATUS_RUNNING;
    timer_schedule_tick(timer);
    timer_schedule_wakeup(timer, 0);
    timers_mark_updated();
}

void timer_reset(Timer *timer, bool return)
{
    timer_pause(timer);
    timer->current_time = timer->length;
    timer->current_amount = timer->base_amount;
    timer->current_tick = 1;
    timer->status = TIMER_STATUS_STOPPED;
    timer_update_energy_per_tick(timer);
    timers_mark_updated();
    if (return)
    {
        main_screen_hide_status_area(false);
    }
}

void timer_restore(Timer *timer, uint16_t seconds_elapsed)
{
    timer->timer = NULL;
    if (timer->status == TIMER_STATUS_RUNNING)
    {
	if (seconds_elapsed >= timer->current_time)
	{
	    timer->current_time = 0;
	    timer->status = TIMER_STATUS_DONE;
	}
	else
	{
	    timer->current_time -= seconds_elapsed;
	}
    }
    if (timer->status == TIMER_STATUS_RUNNING)
    {
	timer_resume(timer);
    }
}

Timer *timer_clone(Timer *timer)
{
    Timer *new_timer = malloc(sizeof(Timer));
    memcpy(new_timer, timer, sizeof(Timer));
    return new_timer;
}

char *timer_vibe_str(TimerVibration vibe, bool shortStr)
{
    switch (vibe)
    {
    case TIMER_VIBE_NONE:
	return "None";
    case TIMER_VIBE_SHORT:
	return shortStr ? "Short" : "Short Pulse";
    case TIMER_VIBE_LONG:
	return shortStr ? "Long" : "Long Pulse";
    case TIMER_VIBE_DOUBLE:
	return shortStr ? "Double" : "Double Pulse";
    case TIMER_VIBE_TRIPLE:
	return shortStr ? "Triple" : "Triple Pulse";
    }
    return "";
}

Timer *timer_create_timer(void)
{
    Timer *timer = malloc(sizeof(Timer));
    timer->length = settings()->timers_duration;
    timer->wakeup_id = -1;
    timer->timer = NULL;
    timer->status = TIMER_STATUS_STOPPED;
    timer->accel = settings()->accel_enabled;
    timer->accel_tick = settings()->accel_tick;
    timer->base_amount = 1;
    timer->current_amount = 1;
    timer->current_tick = 1;
    timer_set_id(timer);
    timer_refresh_info(timer);
    return timer;
}

void timer_update_energy_per_tick(Timer *timer)
{
    uint8_t update = timer->base_amount;
    if (timer->accel) 
    {
        update += timer->current_tick / timer->accel_tick;
    }
    if (settings()->quicken_enabled)
    {
        update *= 2;
    }
    timer->current_amount = update;
}

void timer_time_till_full(Timer *timer) 
{
    uint32_t eta_time = 0;
    Timer *temp_timer = timer_clone(timer);
    eta_time = temp_timer->current_time;
    
    uint8_t energy = settings()->current_energy + temp_timer->current_amount;
    if (energy >= settings()->max_energy) 
    {
        //free(temp_timer);
        timer->full_time = eta_time;
    }
    
    uint8_t additional_ticks = 0;
    while (energy < settings()->max_energy) 
    {
        additional_ticks += 1;
        temp_timer->current_tick += 1;
        timer_update_energy_per_tick(temp_timer);
        energy += temp_timer->current_amount;
    }
    
    eta_time += (additional_ticks * temp_timer->length);
    
    //free(temp_timer);
    timer->full_time = eta_time;
}

void timer_refresh_info(Timer *timer) 
{
    timer_update_energy_per_tick(timer);
    timer_time_till_full(timer);
}

static void timer_tick(void *context)
{
    Timer *timer = (Timer *)context;
    timer->timer = NULL;
    timer->current_time -= 1;
    timer->full_time -= 1;
    if (timer->current_time <= 0)
    {
	timer_finish(timer);
    }
    if (timer->status == TIMER_STATUS_RUNNING)
    {
	timer_schedule_tick(timer);
    }
    timers_mark_updated();
}

static void timer_finish(Timer *timer)
{
    timer->status = TIMER_STATUS_DONE;
    timer_completed_action(timer);
}

static void timer_schedule_tick(Timer *timer)
{
    timer_cancel_tick(timer);
    timer->timer = app_timer_register(1000, timer_tick, (void *)timer);
}

static void timer_cancel_tick(Timer *timer)
{
    if (!timer)
    {
	return;
    }
    if (timer->timer)
    {
	app_timer_cancel(timer->timer);
	timer->timer = NULL;
    }
}

static void timer_schedule_wakeup(Timer *timer, uint16_t offset)
{
    if (!timer)
    {
	return;
    }
    timer_cancel_wakeup(timer);
    time_t wakeup_time = time(NULL);
    wakeup_time += timer->current_time;
    wakeup_time -= 2;
    wakeup_time -= offset;
    timer->wakeup_id = wakeup_schedule(wakeup_time, timer->id, false);
    if (timer->wakeup_id >= 0)
    {
	return;
    }
    switch (timer->wakeup_id)
    {
    case E_RANGE:
    {
	Timer *timer_collision = timers_find_wakeup_collision(timer);
	if (timer_collision)
	{
	}
	else
	{
	}
	timer_schedule_wakeup(timer, offset - 20);
	break;
    }
    case E_OUT_OF_RESOURCES:
    {
	Timer *last_timer = timers_find_last_wakeup();
	if (NULL == last_timer)
	{
	    return;
	}
	if (timer->id == last_timer->id)
	{
	    return;
	}
	else
	{
	    timer_cancel_wakeup(last_timer);
	    timer_schedule_wakeup(timer, 0);
	}
	break;
    }
    case E_INVALID_ARGUMENT:
	break;
    }
}

static void timer_cancel_wakeup(Timer *timer)
{
    if (!timer)
    {
	return;
    }
    if (timer->wakeup_id <= 0)
    {
	return;
    }
    wakeup_cancel(timer->wakeup_id);
    timer->wakeup_id = -1;
}

static void timer_set_id(Timer *timer)
{
    timer->id = rand();
    while (timers_find(timer->id))
    {
	timer->id = rand();
    }
}

static void timer_completed_action(Timer *timer)
{
    TimerVibration current_vibration = settings()->timers_tick_vibration;
    bool finished = false;
    
    //TODO: Add energy refill logic
    settings()->current_energy += timer->current_amount;
    
    if (settings()->current_energy >= settings()->max_energy) 
    {
        current_vibration = settings()->timers_finish_vibration;
        finished = true;
        settings()->current_energy = settings()->max_energy;
    }
    
    switch (current_vibration)
    {
        case TIMER_VIBE_NONE:
	    break;
        case TIMER_VIBE_SHORT:
	        vibes_short_pulse();
	    break;
        case TIMER_VIBE_LONG:
	        vibes_long_pulse();
	    break;
        case TIMER_VIBE_DOUBLE:
        {
	        const uint32_t seg[] = {600, 200, 600};
	        VibePattern pattern = {
	            .durations = seg,
	            .num_segments = ARRAY_LENGTH(seg)};
	        vibes_enqueue_custom_pattern(pattern);
	        break;
        }
        case TIMER_VIBE_TRIPLE:
        {
	        const uint32_t seg[] = {600, 200, 600, 200, 600};
	        VibePattern pattern = {
	            .durations = seg,
	            .num_segments = ARRAY_LENGTH(seg)};
	        vibes_enqueue_custom_pattern(pattern);
	        break;
        }
        default:
	    break;
    }
    timer_update_energy_per_tick(timer);
    if (!finished)
    {
        timer->current_tick +=1;
        timer_start(timer);
    }
    else
    {
        timer_reset(timer, true);
        //main_screen_hide_status_area(true);
    }
    //timers_highlight(timer);
}
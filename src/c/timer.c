#include <pebble.h>
#include "timer.h"

static void timer_tick(void* context);
static void timer_finish(Timer* timer);
static void timer_schedule_tick(Timer* timer);
static void timer_cancel_tick(Timer* timer);
static void timer_schedule_wakeup(Timer* timer, uint16_t offset);
static void timer_cancel_wakeup(Timer* timer);
static void timer_set_id(Timer* timer);
static void timer_completed_action(Timer* timer);

void timer_time_str(uint32_t timer_time, bool showHours, char* str, int str_len) {
	int hours = timer_time / 3600;
	int minutes = (showHours ? (timer_time % 3600) : timer_time) / 60;
	int seconds = (showHours ? (timer_time % 3600) : timer_time) % 60;
	if (showHours) {
		snprintf(str, str_len, "%02d:%02d:%02d", hours, minutes, seconds);
	}
	else {
		snprintf(str, str_len, "%02d:%02d", minutes, seconds);
	}
}

void timer_start(Timer* timer) {
	switch (timer->type) {
		case TIMER_TYPE_TIMER:
			timer->current_time = timer->length;
			break;
		case TIMER_TYPE_STOPWATCH:
			timer->current_time = 0;
			break;
	}
	timer->status = TIMER_STATUS_RUNNING;
	timer_schedule_tick(timer);
	timer_schedule_wakeup(timer, 0);
	//timers_mark_updated();
}

void timer_pause(Timer* timer) {
	timer->status = TIMER_STATUS_PAUSED;
	timer_cancel_tick(timer);
	timer_cancel_wakeup(timer);
	//timers_mark_updated();
}

void timer_resume(Timer* timer) {
	timer->status = TIMER_STATUS_RUNNING;
	timer_schedule_tick(timer);
	timer_schedule_wakeup(timer, 0);
	//timers_mark_updated();
}

void timer_reset(Timer* timer) {
	timer_pause(timer);
	switch (timer->type) {
		case TIMER_TYPE_TIMER:
			timer->current_time = timer->length;
			break;
		case TIMER_TYPE_STOPWATCH:
			timer->current_time = 0;
			break;
	}
	timer->status = TIMER_STATUS_STOPPED;
	//timers_mark_updated();
}

void timer_restore(Timer* timer, uint16_t seconds_elapsed) {
	timer->timer = NULL;
	if (timer->status == TIMER_STATUS_RUNNING) {
		switch (timer->type) {
			case TIMER_TYPE_STOPWATCH:
				timer->current_time += seconds_elapsed;
				break;
			case TIMER_TYPE_TIMER:
				if (seconds_elapsed >= timer->current_time) {
					
				}
				break;
		}
	}
}

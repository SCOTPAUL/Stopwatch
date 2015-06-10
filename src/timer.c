/*
 * timer.c
 * Helper functions for Stopwatch
 */

#include <pebble.h>
#include "timer.h"

unsigned int get_current_time_ms(){
    time_t current_time_s;
    uint16_t current_time_ms;

    time_ms(&current_time_s, &current_time_ms);

    return (1000*current_time_s + current_time_ms);;
}

unsigned int time_since_last_pause_ms(Timer *timer) {
    unsigned int last_paused = (1000*timer->last_paused_s + timer->last_paused_ms);
    return get_current_time_ms() - last_paused;
}

// Return number of ms since the app was last closed
unsigned int get_closed_time(Timer *timer){
    unsigned long current_time = get_current_time_ms();
    unsigned long last_closed = (1000*timer->last_closed_s + timer->last_closed_ms);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Closed for %lums", current_time - last_closed);

    return current_time - last_closed;
}

// Reset timer
void reset_timer(Timer *timer){
    timer->elapsed_ms = timer->elapsed_ms_at_pause = 0;
    time_ms(&timer->last_paused_s, &timer->last_paused_ms);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Time reset");
}

void update_time(Timer *timer){
    if(!timer->paused){
        timer->elapsed_ms = time_since_last_pause_ms(timer) + timer->elapsed_ms_at_pause;
    }
}

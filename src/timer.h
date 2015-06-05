#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#define POLLING_PERIOD 10
#define KEY_TIMER 0

typedef struct Timer {
    bool paused;
    unsigned int elapsed_ms;
    unsigned int elapsed_ms_at_pause;

    time_t last_paused_s;
    uint16_t last_paused_ms;

    time_t last_closed_s;
    uint16_t last_closed_ms;
} Timer;

unsigned int get_current_time_ms();
unsigned int time_since_last_pause_ms(Timer *timer);
void update_time(Timer *timer);

#endif

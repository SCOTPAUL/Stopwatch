/*
 * Timer.c
 * A simple stopwatch timer.
 *
 * Custom font source: http://www.dafont.com/perfect-dos-vga-437.font
 */

#include <pebble.h>
#include "timer.h"

static Window *window;
static TextLayer *timer_layer;
static TextLayer *centis_layer;
static GFont s_timer_font;

static Timer timer;

// Print stopwatch time to screen in format "mm:ss dc"
static void print_time(void *data){
    app_timer_register(POLLING_PERIOD, print_time, NULL);
    update_time(&timer);

    static char text_time[] = "00:00";
    static char centis_time[] = "00";

    int printing_time = timer.elapsed_ms;
    int minutes = printing_time / 60000;
    printing_time -= minutes * 60000;
    int seconds = printing_time / 1000;
    printing_time -= seconds * 1000;
    int centis = printing_time / 10;

    snprintf(text_time, sizeof("00:00"), "%02d:%02d", minutes, seconds);
    snprintf(centis_time, sizeof("00"), "%02d", centis);

    text_layer_set_text(timer_layer, text_time);
    text_layer_set_text(centis_layer, centis_time);
}

// Reset timing
static void reset_timer(Timer *timer){
    timer->elapsed_ms = timer->elapsed_ms_at_pause = 0;
    time_ms(&timer->last_paused_s, &timer->last_paused_ms);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Time reset");
}

// Pause timing
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    timer.paused = !timer.paused;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Paused: %s", timer.paused?"true":"false");
    if(!timer.paused){
        time_ms(&timer.last_paused_s, &timer.last_paused_ms);
        timer.elapsed_ms_at_pause = timer.elapsed_ms;
    }
}

// Reset the time to 0 and print the zeroed time to the screen.
static void up_click_handler(ClickRecognizerRef recognizer, void *context){
    reset_timer(&timer);
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    timer_layer = text_layer_create((GRect) {
        .origin = { 0, 40 },
        .size = { bounds.size.w, 50 }
    });

    s_timer_font = fonts_load_custom_font(
        resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_48)
    );

    text_layer_set_background_color(timer_layer, GColorClear);
    text_layer_set_font(timer_layer, s_timer_font);
    text_layer_set_text(timer_layer, "00:00");
    text_layer_set_text_alignment(timer_layer, GTextAlignmentCenter);


    centis_layer = text_layer_create((GRect){
        .origin = {0, 80},
        .size = {bounds.size.w, 50 }
    });

    text_layer_set_background_color(centis_layer, GColorClear);
    text_layer_set_font(centis_layer, s_timer_font);
    text_layer_set_text(centis_layer, "00");
    text_layer_set_text_alignment(centis_layer, GTextAlignmentCenter);

    // Print the time before the window is pushed onto the stack to avoid
    // seeing an empty screen
    print_time(NULL);

    layer_add_child(window_layer, text_layer_get_layer(timer_layer));
    layer_add_child(window_layer, text_layer_get_layer(centis_layer));

}

static void window_unload(Window *window) {
    text_layer_destroy(timer_layer);
    text_layer_destroy(centis_layer);

    fonts_unload_custom_font(s_timer_font);
}

// Return the number of milliseconds since the app was last closed.
static unsigned int get_closed_time(){
    unsigned long current_time = get_current_time_ms();
    unsigned long last_closed = (1000*timer.last_closed_s + timer.last_closed_ms);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Closed for %lums", current_time - last_closed);

    return current_time - last_closed;
}

static void init(void) {
    // Set up the window
    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });

    const bool fullscreen = true;
    const bool animated = true;
    window_set_fullscreen(window, fullscreen);

    // Set up the timer, and read persistant values from memory.
    app_timer_register(POLLING_PERIOD, print_time, NULL);

    // Ensure that timer.paused is true on first run of app.
    if(persist_exists(KEY_TIMER)) persist_read_data(KEY_TIMER, &timer, sizeof(timer));
    else timer.paused = true;

    // If the app wasn't paused when last closed...
    if(!timer.paused){
        timer.elapsed_ms += get_closed_time();
    }

    window_stack_push(window, animated);
}

static void deinit(void) {
    window_destroy(window);

    // Write out time last to reduce inaccuracy.
    time_ms(&timer.last_closed_s, &timer.last_closed_ms);
    persist_write_data(KEY_TIMER, &timer, sizeof(timer));
}

int main(void) {
    init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

    app_event_loop();
    deinit();
}

/*
 * Timer.c
 * A simple stopwatch timer.
 *
 * Custom font source: http://www.dafont.com/perfect-dos-vga-437.font
 */

#include <pebble.h>
#define POLLING_PERIOD 100

#define KEY_ELAPSED_TIME 0
#define KEY_PAUSED 1
#define KEY_LAST_CLOSED_S 2
#define KEY_LAST_CLOSED_MS 3

static Window *window;
static TextLayer *timer_layer;
static TextLayer *decis_layer;
static GFont s_timer_font;

static bool paused;
static unsigned int elapsed_ms;

static time_t last_closed_s;
static uint16_t last_closed_ms;


static void print_time(){
    static char text_time[] = "00:00";
    static char decis_time[] = "0";

    int printing_time = elapsed_ms;
    int minutes = printing_time / 60000;
    printing_time -= minutes * 60000;
    int seconds = printing_time / 1000;
    printing_time -= seconds * 1000;
    int decis = printing_time / 100;

    snprintf(text_time, sizeof("00:00"), "%02d:%02d", minutes, seconds);
    snprintf(decis_time, sizeof("0"), "%d", decis);

    text_layer_set_text(timer_layer, text_time);
    text_layer_set_text(decis_layer, decis_time);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    paused = !paused;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Paused: %s", paused?"true":"false");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context){
    elapsed_ms = 0;
    print_time();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Time reset");
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


    decis_layer = text_layer_create((GRect){
        .origin = {0, 80},
        .size = {bounds.size.w, 50 }
    });

    text_layer_set_background_color(decis_layer, GColorClear);
    text_layer_set_font(decis_layer, s_timer_font);
    text_layer_set_text(decis_layer, "00");
    text_layer_set_text_alignment(decis_layer, GTextAlignmentCenter);

    print_time();

    layer_add_child(window_layer, text_layer_get_layer(timer_layer));
    layer_add_child(window_layer, text_layer_get_layer(decis_layer));

}

static void window_unload(Window *window) {
    text_layer_destroy(timer_layer);
    text_layer_destroy(decis_layer);

    fonts_unload_custom_font(s_timer_font);
}


static void update_time(void *data){
    if(!paused){
        elapsed_ms += POLLING_PERIOD;
        print_time();
    }
    app_timer_register(POLLING_PERIOD, update_time, NULL);
}

static unsigned int get_closed_time(){
    time_t current_time_s;
    uint16_t current_time_ms;

    time_ms(&current_time_s, &current_time_ms);

    unsigned long current_time = (1000*current_time_s + current_time_ms);
    unsigned long last_closed = (1000*last_closed_s + last_closed_ms);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Closed for %lums", current_time - last_closed);

    return current_time - last_closed;

}

static void init(void) {
    elapsed_ms = persist_read_int(KEY_ELAPSED_TIME);

    if(persist_exists(KEY_PAUSED)) paused = persist_read_bool(KEY_PAUSED);
    else paused = true;

    if(!paused){
        last_closed_s = persist_read_int(KEY_LAST_CLOSED_S);
        last_closed_ms = persist_read_int(KEY_LAST_CLOSED_MS);
        elapsed_ms += get_closed_time();
    }


    app_timer_register(POLLING_PERIOD, update_time, NULL);

    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });

    const bool fullscreen = true;
    const bool animated = true;
    window_set_fullscreen(window, fullscreen);
    window_stack_push(window, animated);
}

static void deinit(void) {
    persist_write_int(KEY_ELAPSED_TIME, elapsed_ms);
    persist_write_bool(KEY_PAUSED, paused);

    window_destroy(window);
    time_ms(&last_closed_s, &last_closed_ms);
    persist_write_int(KEY_LAST_CLOSED_S, last_closed_s);
    persist_write_int(KEY_LAST_CLOSED_MS, last_closed_ms);

}

int main(void) {
    init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

    app_event_loop();
    deinit();
}

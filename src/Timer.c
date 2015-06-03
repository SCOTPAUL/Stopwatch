/*
 * Timer.c
 * A simple stopwatch timer.
 *
 * Custom font source: http://www.dafont.com/perfect-dos-vga-437.font
 */

#include <pebble.h>
#define POLLING_PERIOD 10

#define KEY_ELAPSED_TIME 0
#define KEY_PAUSED 1

static Window *window;
static TextLayer *timer_layer;
static TextLayer *millis_layer;
static GFont s_timer_font;

static bool paused;
static unsigned int elapsed_ms;

static void print_time(){
    static char text_time[] = "00:00";
    static char decs_time[] = "00";

    int printing_time = elapsed_ms;
    int minutes = printing_time / 60000;
    printing_time -= minutes * 60000;
    int seconds = printing_time / 1000;
    printing_time -= seconds * 1000;
    int decs = printing_time / 10;

    snprintf(text_time, sizeof("00:00"), "%02d:%02d", minutes, seconds);
    snprintf(decs_time, sizeof("00"), "%02d", decs);

    text_layer_set_text(timer_layer, text_time);
    text_layer_set_text(millis_layer, decs_time);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    paused = !paused;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Paused: %i", paused);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context){
    elapsed_ms = 0;
    print_time();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Time Reset");
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


    millis_layer = text_layer_create((GRect){
        .origin = {0, 80},
        .size = {bounds.size.w, 50 }
    });

    text_layer_set_background_color(millis_layer, GColorClear);
    text_layer_set_font(millis_layer, s_timer_font);
    text_layer_set_text(millis_layer, "00");
    text_layer_set_text_alignment(millis_layer, GTextAlignmentCenter);

    print_time();

    layer_add_child(window_layer, text_layer_get_layer(timer_layer));
    layer_add_child(window_layer, text_layer_get_layer(millis_layer));

}

static void window_unload(Window *window) {
    text_layer_destroy(timer_layer);

    fonts_unload_custom_font(s_timer_font);
}


static void update_time(void *data){
    if(!paused){
        elapsed_ms += POLLING_PERIOD;
        print_time();
    }
    app_timer_register(POLLING_PERIOD, update_time, NULL);
}

static void init(void) {
    elapsed_ms = persist_read_int(KEY_ELAPSED_TIME);
    paused = persist_read_bool(KEY_PAUSED);

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
}

int main(void) {
    init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

    app_event_loop();
    deinit();
}

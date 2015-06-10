/*
 * stopwatch.c
 * A simple stopwatch timer.
 *
 * Custom font source: http://www.dafont.com/perfect-dos-vga-437.font
 */

#include <pebble.h>
#include "timer.h"

#define TIME_KEY 0

static Window *window;
static TextLayer *timer_layer;
static TextLayer *centis_layer;
static GFont s_timer_font;

static ActionBarLayer *action_bar;
static GBitmap *s_play_icon;
static GBitmap *s_pause_icon;
static GBitmap *s_select_button_icon;
static GBitmap *s_share_icon;
static GBitmap *s_refresh_icon;

static Timer timer;

// Print stopwatch time to screen in format "mm:ss cc"
static void print_time(){
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

// Wrapper around print_time to set up printing every POLLING_PERIOD ms
static void print_loop(void *data){
    app_timer_register(POLLING_PERIOD, print_loop, NULL);

    if(!timer.paused){
        print_time();
    }
}

// Pause timing
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    timer.paused = !timer.paused;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Paused: %s", timer.paused?"true":"false");

    if(!timer.paused){
        time_ms(&timer.last_paused_s, &timer.last_paused_ms);
        timer.elapsed_ms_at_pause = timer.elapsed_ms;
        s_select_button_icon = s_pause_icon;
    }
    else{
        s_select_button_icon = s_play_icon;
    }

    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, s_select_button_icon);
}

// Reset the time to 0 and print the zeroed time to the screen.
static void up_click_handler(ClickRecognizerRef recognizer, void *context){
    reset_timer(&timer);
    print_time();
}

// Write out the current elapsed time in ms to the AppMessage outbox
static void down_click_handler(ClickRecognizerRef recognizer, void *context){
    const uint32_t time_ms = timer.elapsed_ms;

    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint32(iter, TIME_KEY, time_ms);
    dict_write_end(iter);
    app_message_outbox_send();
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

// Only show the share icon if the phone is connected
static void bluetooth_connection_handler(bool connected){
    if(connected){
        action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, s_share_icon);
    }
    else{
        action_bar_layer_clear_icon(action_bar, BUTTON_ID_DOWN);
    }
}

// Setup the action bar on the side of the app
static void action_bar_init(){
    s_play_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PLAY);
    s_pause_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PAUSE);
    s_refresh_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_REFRESH);
    s_share_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SHARE);

    if(timer.paused) s_select_button_icon = s_play_icon;
    else s_select_button_icon = s_pause_icon;

    action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, s_refresh_icon);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, s_select_button_icon);

    if(bluetooth_connection_service_peek()){
        action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, s_share_icon);
    }
    bluetooth_connection_service_subscribe(bluetooth_connection_handler);
}

static void action_bar_deinit(){
    gbitmap_destroy(s_play_icon);
    gbitmap_destroy(s_pause_icon);
    gbitmap_destroy(s_refresh_icon);
    gbitmap_destroy(s_share_icon);
    bluetooth_connection_service_unsubscribe();
    action_bar_layer_destroy(action_bar);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    timer_layer = text_layer_create((GRect) {
        .origin = { 0, 40 },
        .size = { bounds.size.w - ACTION_BAR_WIDTH, 50 }
    });

    s_timer_font = fonts_load_custom_font(
        resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_42)
    );

    text_layer_set_background_color(timer_layer, GColorClear);
    text_layer_set_font(timer_layer, s_timer_font);
    text_layer_set_text(timer_layer, "00:00");
    text_layer_set_text_alignment(timer_layer, GTextAlignmentCenter);


    centis_layer = text_layer_create((GRect){
        .origin = {0, 80},
        .size = {bounds.size.w - ACTION_BAR_WIDTH, 50 }
    });

    text_layer_set_background_color(centis_layer, GColorClear);
    text_layer_set_font(centis_layer, s_timer_font);
    text_layer_set_text(centis_layer, "00");
    text_layer_set_text_alignment(centis_layer, GTextAlignmentCenter);

    action_bar = action_bar_layer_create();
    action_bar_layer_add_to_window(action_bar, window);
    action_bar_layer_set_click_config_provider(action_bar, click_config_provider);

    // Load actionbar icons
    action_bar_init();

    // Print the time before the window is pushed onto the stack to avoid
    // seeing an empty screen
    print_time();

    layer_add_child(window_layer, text_layer_get_layer(timer_layer));
    layer_add_child(window_layer, text_layer_get_layer(centis_layer));

}

static void window_unload(Window *window) {
    text_layer_destroy(timer_layer);
    text_layer_destroy(centis_layer);

    // Unload action bar
    action_bar_deinit();
    fonts_unload_custom_font(s_timer_font);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context){
    APP_LOG(APP_LOG_LEVEL_INFO, "Message recieved!");
}

static void inbox_dropped_callback(AppMessageResult reason, void *context){
    APP_LOG(APP_LOG_LEVEL_ERROR, "Mesage dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context){
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
    vibes_double_pulse();
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context){
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
    vibes_short_pulse();
}

static void register_appmessage_callbacks(){
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
}

static void init(void) {
    // Set up the window
    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });

    const bool animated = true;

    // Set the window fullscreen if on aplite
    // (basalt windows are fullscreen by default)
    #ifdef PBL_PLATFORM_APLITE
        const bool fullscreen = true;
        window_set_fullscreen(window, fullscreen);
    #endif

    // Setup AppMessage
    register_appmessage_callbacks();
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

    // Setup print_loop to run every POLLING_PERIOD ms
    app_timer_register(POLLING_PERIOD, print_loop, NULL);

    // Ensure that timer.paused is true on first run of app.
    if(persist_exists(KEY_TIMER)) persist_read_data(KEY_TIMER, &timer, sizeof(timer));
    else timer.paused = true;

    // If the app wasn't paused when last closed...
    if(!timer.paused){
        timer.elapsed_ms += get_closed_time(&timer);
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

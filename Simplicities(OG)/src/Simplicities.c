//this code is a modified version of "simplicity" from  SDK\Examples
//Originally by Pebble, Modified & annotated By Scott Gordon
//an amount of code was selectively modified from other examples.

#include "pebble.h"

Window *window;                           //display window
static TextLayer *text_day_layer;         //text layer for day
static TextLayer *text_date_layer;        //month and date
static TextLayer *text_time_layer;        //current time
static TextLayer *temperature_layer;      //temp
static TextLayer *city_layer; 		      //location
static Layer *line_layer;                 //pretty line
static Layer *line2_layer;

// static, used by system.
static char time_text[] = "00::00";        //extra colon placeholder
static char date_text[] = "Xxxxxxxxx 00";
static char day_text[] = "xxxxxxxxxx";

static GBitmap *BT_image = NULL;
static BitmapLayer *BT_icon_layer;          //Layer for the BT connection
static GBitmap *Batt_image = NULL;
static BitmapLayer *Batt_icon_layer;        //Layer for the Battery status

static AppSync sync;
static uint8_t sync_buffer[64];

enum WeatherKey {
    WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
    WEATHER_CITY_KEY = 0x2,         // TUPLE_CSTRING
};

bool tock=true;                            //flag for 'tick tock' feature

#define day_frame         (GRect(4, 11, 144-8, 168-68))
#define date_frame        (GRect(4, 33, 144-8, 168-68))
#define line_frame        (GRect(4, 62, 110, 2))
#define time_frame        (GRect(4, 60, 200-9, 600-92))
#define line2_frame       (GRect(4, 111, 130, 2))
#define BT_frame          (GRect(115,  52, 30, 168-146))
#define Batt_frame        (GRect(110,  0, 40, 168-146))
#define LOCATION_FRAME    (GRect(4, 114, 144-8, 168-68))
#define TEMPERATURE_FRAME (GRect(4, 138, 144-8, 168-68))

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
    switch (key) {
            
        case WEATHER_TEMPERATURE_KEY:
            text_layer_set_text(temperature_layer, new_tuple->value->cstring);
            layer_mark_dirty(text_layer_get_layer(temperature_layer));
            break;
            
        case WEATHER_CITY_KEY:
            text_layer_set_text(city_layer, new_tuple->value->cstring);
            layer_mark_dirty(text_layer_get_layer(city_layer));
            break;
    }
}

static void send_cmd(void) {
    Tuplet value = TupletInteger(1, 1);
	
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
	
    if (iter == NULL) {
		return;
    }
	
    dict_write_tuplet(iter, &value);
    dict_write_end(iter);
	
    app_message_outbox_send();
}

static void handle_battery(BatteryChargeState charge_state) {
    if (charge_state.is_charging) {
	  	Batt_image = gbitmap_create_with_resource(RESOURCE_ID_BATT_CHAR);
    }
    else{
        Batt_image = gbitmap_create_with_resource(RESOURCE_ID_BT_OFF);
        
        if (charge_state.charge_percent<=10)
        {
            Batt_image = gbitmap_create_with_resource(RESOURCE_ID_BATT_EMPTY);
        }
    }
    bitmap_layer_set_bitmap(Batt_icon_layer, Batt_image);
}

static void handle_bluetooth(bool connected){
    if(connected)
	{
        BT_image = gbitmap_create_with_resource(RESOURCE_ID_BT_CONNECTED);
    }
	else{
        vibes_double_pulse();
        BT_image = gbitmap_create_with_resource(RESOURCE_ID_BT_OFF);
    }
    bitmap_layer_set_bitmap(BT_icon_layer, BT_image);
}

static void line_layer_callback(Layer *layer, GContext* ctx) {
    graphics_context_set_fill_color(ctx, GColorWhite);              //is white
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone); //is line
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
    
    char *time_format;                         //placeholder for formatting
    strftime(date_text, sizeof(date_text), "%B %e", tick_time);  //get date
    text_layer_set_text(text_date_layer, date_text); //set date text
    
    strftime(day_text, sizeof(day_text), "%A", tick_time); //get day
    strcat(day_text,","); //add comma
    text_layer_set_text(text_day_layer, day_text); //set day text
    
    if(clock_is_24h_style()){
        if (tock){
            time_format = "%H: %M";         //24h, colon left, minutes
            tock=false;                     //tock set to 'tick'
        }
        else{
            time_format = "%H :%M";     //24h, colon right, minutes
            tock=true;
        }
    }
    else
        if (tock){
            time_format = "%I: %M";         //12h, colon left, minutes
            tock=false;
        }
        else{
            time_format = "%I :%M";         //12h, colon right, minutes
            tock=true;                      //tock set to 'tock'
        }
    
    strftime(time_text, sizeof(time_text), time_format, tick_time);  //get time
    
    if (!clock_is_24h_style() && (time_text[0] == '0')) {    //remove leading 0
        memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }
    text_layer_set_text(text_time_layer, time_text);     //set time
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    
    text_day_layer = text_layer_create(day_frame);
    text_layer_set_text_color(text_day_layer, GColorWhite);
    text_layer_set_background_color(text_day_layer, GColorClear);
    text_layer_set_font(text_day_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
    layer_add_child(window_layer, text_layer_get_layer(text_day_layer));
    
    text_date_layer = text_layer_create(date_frame);
    text_layer_set_text_color(text_date_layer, GColorWhite);
    text_layer_set_background_color(text_date_layer, GColorClear);
    text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
    layer_add_child(window_layer, text_layer_get_layer(text_date_layer));
    
    line_layer = layer_create(line_frame);
    layer_set_update_proc(line_layer, line_layer_callback);
    layer_add_child(window_layer, line_layer);
    
    text_time_layer = text_layer_create(time_frame);
    text_layer_set_text_color(text_time_layer, GColorWhite);
    text_layer_set_background_color(text_time_layer, GColorClear);
    text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(text_time_layer));
    
    line2_layer = layer_create(line2_frame);
    layer_set_update_proc(line2_layer, line_layer_callback);
    layer_add_child(window_layer, line2_layer);
    
    battery_state_service_subscribe(handle_battery);
    Batt_icon_layer = bitmap_layer_create(Batt_frame);
    handle_battery(battery_state_service_peek());
    layer_add_child(window_layer, bitmap_layer_get_layer(Batt_icon_layer));
    
    bluetooth_connection_service_subscribe(handle_bluetooth);
    BT_icon_layer = bitmap_layer_create(BT_frame);
    handle_bluetooth(bluetooth_connection_service_peek());
    bitmap_layer_set_bitmap(BT_icon_layer, BT_image);
    layer_add_child(window_layer, bitmap_layer_get_layer(BT_icon_layer));
    
    temperature_layer = text_layer_create(TEMPERATURE_FRAME);
    text_layer_set_text_color(temperature_layer, GColorWhite);
    text_layer_set_background_color(temperature_layer, GColorClear);
    text_layer_set_font(temperature_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
    text_layer_set_text_alignment(temperature_layer, GTextAlignmentLeft);
    layer_add_child(window_layer, text_layer_get_layer(temperature_layer));
    
    city_layer = text_layer_create(LOCATION_FRAME);
    text_layer_set_text_color(city_layer, GColorWhite);
    text_layer_set_background_color(city_layer, GColorClear);
    text_layer_set_font(city_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
    text_layer_set_text_alignment(city_layer, GTextAlignmentLeft);
    layer_add_child(window_layer, text_layer_get_layer(city_layer));
	
    Tuplet initial_values[] = {
        TupletCString(WEATHER_TEMPERATURE_KEY, ""),
        TupletCString(WEATHER_CITY_KEY, ""),
    };
    
    app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values,
                  ARRAY_LENGTH(initial_values), sync_tuple_changed_callback,
                  sync_error_callback, NULL);
    
    send_cmd();
}

static void init(void) {
    tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
    window = window_create();
    window_set_background_color(window, GColorBlack);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
    });
    
    const int inbound_size = 64;
    const int outbound_size = 64;
    app_message_open(inbound_size, outbound_size);
    
    window_stack_push(window, true);                    //animated
}

static void deinit(void) {
    
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    
    layer_remove_from_parent(bitmap_layer_get_layer(BT_icon_layer));
    gbitmap_destroy(BT_image);
    bitmap_layer_destroy(BT_icon_layer);
    
    layer_remove_from_parent(bitmap_layer_get_layer(Batt_icon_layer));
    gbitmap_destroy(Batt_image);
    bitmap_layer_destroy(Batt_icon_layer);
    
    
    text_layer_destroy(temperature_layer);
    text_layer_destroy(city_layer);
    text_layer_destroy(text_day_layer);
    text_layer_destroy(text_date_layer);
    text_layer_destroy(text_time_layer);
    window_destroy(window);
    
    app_sync_deinit(&sync);
    
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}

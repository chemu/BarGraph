#include <pebble.h>
#include <config.h>



static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer;
static Layer *s_bar_layer;
static GBitmap *s_bt_bitmap;
static Layer *s_bt_layer;
static GFont s_time_font, s_date_font; 
static int s_temperature, s_pop, s_batt;
static char s_time_buffer[8];
static char s_date_buffer[8];

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
//  strncpy(s_buffer, "23:59", 8);
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_time_buffer);
  
  strftime(s_date_buffer, sizeof(s_date_buffer),"%a %d", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void byte_set_bit(uint8_t *byte, uint8_t bit, uint8_t value) {
  *byte ^= (-value ^ *byte) & (1 << bit);
}

static void set_pixel_color(GBitmapDataRowInfo info, GPoint point, GColor color) {
#if defined(PBL_COLOR)
  // Write the pixel's byte color
  memset(&info.data[point.x], color.argb, 1);
#elif defined(PBL_BW)
  // Find the correct byte, then set the appropriate bit
  uint8_t byte = point.x / 8;
  uint8_t bit = point.x % 8; 
  byte_set_bit(&info.data[byte], bit, gcolor_equal(color, GColorWhite) ? 1 : 0);
#endif
}

static void layer_update_proc(Layer *layer, GContext *ctx) {
  int length_red, length_blue, length_green, x, y;
  GColor bar_color;
  
  // Get the framebuffer
  GBitmap *fb = graphics_capture_frame_buffer(ctx);

  // Manipulate the image data...
  
  GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, BAR_BEGINNING);
  length_red =  s_temperature * (info.max_x - info.min_x) / RANGE_RED;
  length_green = s_batt * (info.max_x - info.min_x) / RANGE_GREEN;
  length_blue = s_pop * (info.max_x - info.min_x) / RANGE_BLUE;
  int temp_red = length_red + (info.max_x - info.min_x);
//  int bar_length = info.max_x - info.min_x;

  // Iterate over desired rows
  for(y = BAR_BEGINNING; y < BAR_END; y++) {
    // Get this row's range and data
    info = gbitmap_get_data_row_info(fb, y);
    
    // Iterate over all visible columns
    if (0 <= length_red) {
      for(x = info.min_x; x < info.max_x; x++) {
        bar_color.argb = 0b11000000;
        
        if (x < length_red) { bar_color.argb = bar_color.argb | 0b00110000; }
        if (x < length_green) { bar_color.argb = bar_color.argb | 0b00001100; }
        if (x < length_blue) { bar_color.argb = bar_color.argb | 0b00000011; }
        
        // Manipulate the pixel at x,y
        set_pixel_color(info, GPoint(x, y), bar_color);
      }
    }
    else {
      for(x = info.min_x; x < info.max_x; x++) {
        bar_color.argb = 0b11000000;
        
        if (x > temp_red) { bar_color.argb = bar_color.argb | 0b00110000; }
        if (x < length_green) { bar_color.argb = bar_color.argb | 0b00001100; }
        if (x < length_blue) { bar_color.argb = bar_color.argb | 0b00000011; }

        // Manipulate the pixel at x,y
        set_pixel_color(info, GPoint(x, y), bar_color);
      }
    }
  }
  
  // Finally, release the framebuffer
  graphics_release_frame_buffer(ctx, fb);
}

static void bt_handler(bool connected) {
  if(connected) {
    layer_set_hidden(s_bt_layer, true);
  } else {
    vibes_double_pulse();
    layer_set_hidden(s_bt_layer, false);
  }
}

static void bt_update_proc(Layer *layer, GContext *ctx) {
//  if(!data_get_boolean_setting(DataKeyBTIndicator)) {
//    return;
//  }

  // Draw it white
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_bt_bitmap, gbitmap_get_bounds(s_bt_bitmap));

  // Swap to FG color
//  GBitmap *fb = graphics_capture_frame_buffer(ctx);
//  universal_fb_swap_colors(fb, layer_get_frame(layer), GColorWhite, data_get_foreground_color());
//  graphics_release_frame_buffer(ctx, fb);
}

static void main_window_load(Window *window) {
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_time_layer = text_layer_create(GRect(HOUR_X_OFFSET, HOUR_Y_OFFSET, bounds.size.w, HOUR_HEIGHT));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  if (BIG_NUMBERS) {
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SFSHC_74));
  }
  else {
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SF_SQUARE_HEAD_45));
  }
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
    
  // Create date layer
  s_date_layer = text_layer_create(GRect(0, DATE_Y_OFFSET, bounds.size.w, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SF_SQUARE_HEAD_20));
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentRight));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  
  // Create BT layer
  s_bt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_WHITE);
  GRect bitmap_bounds = gbitmap_get_bounds(s_bt_bitmap);

  s_bt_layer = layer_create(GRect(
    (bounds.size.w - bitmap_bounds.size.w) / 2,
    BT_Y_OFFSET, bitmap_bounds.size.w, bitmap_bounds.size.h));
  
  layer_set_update_proc(s_bt_layer, bt_update_proc);
  layer_add_child(window_layer, s_bt_layer);
  
  // Create bar layer
  s_bar_layer = layer_create(bounds);
  layer_add_child(window_layer,s_bar_layer);
  layer_set_update_proc(s_bar_layer, layer_update_proc);
  layer_set_hidden(s_bt_layer, connection_service_peek_pebble_app_connection());

}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  fonts_unload_custom_font(s_time_font);
  text_layer_destroy(s_date_layer);
  fonts_unload_custom_font(s_date_font);
  layer_destroy(s_bar_layer);
  layer_destroy(s_bt_layer);
  gbitmap_destroy(s_bt_bitmap);

}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
    // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
      // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

      // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

      // Send the message!
    app_message_outbox_send();
  }
}

static void battery_state_handler(BatteryChargeState charge) {
//  static char batt_buffer[8];
  
  s_batt = charge.charge_percent;
  layer_mark_dirty(s_bar_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  
    // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
  Tuple *pop_tuple = dict_find(iterator, KEY_POP);
  
  s_temperature = temp_tuple->value->int32;
//  s_temperature = 10;
  s_pop = pop_tuple->value->int32;
//  s_pop = 90;
  
  layer_mark_dirty(s_bar_layer);
  
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init () {
  const int inbox_size = 4;
  const int outbox_size = 4;
  
  s_temperature = 0;
  s_pop = 0;
  BatteryChargeState state = battery_state_service_peek();
  s_batt = state.charge_percent;
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load, 
    .unload = main_window_unload 
  });
  
    // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
    // Open AppMessage
  app_message_open(inbox_size, outbox_size);


  window_stack_push(s_main_window, true);
  update_time();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_state_handler);
  connection_service_subscribe((ConnectionHandlers) {
      .pebble_app_connection_handler = bt_handler
    });
  window_set_background_color(s_main_window, GColorBlack);
  
}  

static void deinit () {
  window_destroy(s_main_window);
}

int main (void) {
  init();
  app_event_loop();
  deinit();
}
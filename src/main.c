#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_date_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static GBitmap *hour_hand_inner_bitmap;
static GBitmap *hour_hand_outer_bitmap;
static GBitmap *minute_hand_inner_bitmap;
static GBitmap *minute_hand_outer_bitmap;
static GBitmap *second_hand_bitmap;

static RotBitmapLayer *hour_hand_inner_layer;
static RotBitmapLayer *hour_hand_outer_layer;
static RotBitmapLayer *minute_hand_inner_layer;
static RotBitmapLayer *minute_hand_outer_layer;
static RotBitmapLayer *second_hand_layer;

static bool display_seconds;

enum {
  KEY_SECONDS = 0
};

static void update_date(struct tm *t) {
  static char buffer[] = "00";

  snprintf(buffer, sizeof("00"), "%d", t->tm_mday);
  text_layer_set_text(s_date_layer, buffer);
}

static void update_second(struct tm *t) {
  int32_t second_angle = (int32_t)(t->tm_sec * TRIG_MAX_ANGLE/60);

  rot_bitmap_layer_set_angle(second_hand_layer, second_angle);
}

static void update_minute(struct tm *t) {
  int32_t minute_angle = (int32_t)((t->tm_min + t->tm_sec/60.0) * TRIG_MAX_ANGLE/60);

  rot_bitmap_layer_set_angle(minute_hand_inner_layer, minute_angle);
  rot_bitmap_layer_set_angle(minute_hand_outer_layer, minute_angle);
}

static void update_hour(struct tm *t) {
  int32_t hour_angle = (int32_t)((t->tm_hour + t->tm_min/60.0) * TRIG_MAX_ANGLE/24);

  rot_bitmap_layer_set_angle(hour_hand_inner_layer, hour_angle);
  rot_bitmap_layer_set_angle(hour_hand_outer_layer, hour_angle);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  if (display_seconds) {
    update_second(tick_time);
  }
  update_minute(tick_time);
  update_hour(tick_time);
  update_date(tick_time);
}

static void rotate_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (display_seconds) {
    update_second(tick_time);
  }

  // update minute handle only every 10 sec
  if (tick_time->tm_sec%10 == 0)
    update_minute(tick_time);

  // update hour handle only every 10 min
  if (units_changed & MINUTE_UNIT)
    if (tick_time->tm_min%10 == 0)
      update_hour(tick_time);

  // update date only when it changes
  if (units_changed & DAY_UNIT)
    update_date(tick_time);
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
      case KEY_SECONDS:
        if (strcmp(t->value->cstring, "on") == 0) {
          APP_LOG(APP_LOG_LEVEL_DEBUG, "set seconds on");
          persist_write_bool(KEY_SECONDS, true);
          display_seconds = true;
        } else if (strcmp(t->value->cstring, "off") == 0) {
          APP_LOG(APP_LOG_LEVEL_DEBUG, "set seconds off");
          persist_write_bool(KEY_SECONDS, false);
          display_seconds = false;
        }
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
        break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "message dropped");
}

static void outbox_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "message send failed");
}

static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "message sent");
}

static void main_window_load(Window *window) {
  GRect bounds;

  display_seconds = true;
  
  // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 143, 167));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));

  // create text layer for date display
  s_date_layer = text_layer_create(GRect(114, 74, 12, 14));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_text(s_date_layer, "00");
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

  // create hour hand layer
  hour_hand_outer_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HOUR_HAND_OUTER);
  hour_hand_outer_layer = rot_bitmap_layer_create(hour_hand_outer_bitmap);
  bounds = layer_get_bounds((Layer*)hour_hand_outer_layer);
  bounds.origin.x = -13;
  bounds.origin.y = 0;
  layer_set_bounds((Layer*)hour_hand_outer_layer, bounds);
  rot_bitmap_set_compositing_mode(hour_hand_outer_layer, GCompOpAnd);
  layer_add_child(window_get_root_layer(window), (Layer*)hour_hand_outer_layer);
  rot_bitmap_layer_set_angle(hour_hand_outer_layer, 0);

  hour_hand_inner_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HOUR_HAND_INNER);
  hour_hand_inner_layer = rot_bitmap_layer_create(hour_hand_inner_bitmap);
  bounds = layer_get_bounds((Layer*)hour_hand_inner_layer);
  bounds.origin.x = -13;
  bounds.origin.y = 0;
  layer_set_bounds((Layer*)hour_hand_inner_layer, bounds);
  rot_bitmap_set_compositing_mode(hour_hand_inner_layer, GCompOpSet);
  layer_add_child(window_get_root_layer(window), (Layer*)hour_hand_inner_layer);
  rot_bitmap_layer_set_angle(hour_hand_inner_layer, 0);

  // create minute hand layer
  minute_hand_outer_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MINUTE_HAND_OUTER);
  minute_hand_outer_layer = rot_bitmap_layer_create(minute_hand_outer_bitmap);
  layer_add_child((Layer*)minute_hand_outer_layer, (Layer*)minute_hand_outer_layer);
  bounds = layer_get_bounds((Layer*)minute_hand_outer_layer);
  bounds.origin.x = -13;
  bounds.origin.y = 0;
  layer_set_bounds((Layer*)minute_hand_outer_layer, bounds);
  rot_bitmap_set_compositing_mode(minute_hand_outer_layer, GCompOpAnd);
  layer_add_child(window_get_root_layer(window), (Layer*)minute_hand_outer_layer);
  rot_bitmap_layer_set_angle(minute_hand_outer_layer, 0);

  minute_hand_inner_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MINUTE_HAND_INNER);
  minute_hand_inner_layer = rot_bitmap_layer_create(minute_hand_inner_bitmap);
  bounds = layer_get_bounds((Layer*)minute_hand_inner_layer);
  bounds.origin.x = -13;
  bounds.origin.y = 0;
  layer_set_bounds((Layer*)minute_hand_inner_layer, bounds);
  rot_bitmap_set_compositing_mode(minute_hand_inner_layer, GCompOpSet);
  layer_add_child(window_get_root_layer(window), (Layer*)minute_hand_inner_layer);
  rot_bitmap_layer_set_angle(minute_hand_inner_layer, 0);

  // create second hand layer
  second_hand_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SECOND_HAND);
  second_hand_layer = rot_bitmap_layer_create(second_hand_bitmap);
  bounds = layer_get_bounds((Layer*)second_hand_layer);
  bounds.origin.x = -12;
  bounds.origin.y = 0;
  layer_set_bounds((Layer*)second_hand_layer, bounds);
  rot_bitmap_set_compositing_mode(second_hand_layer, GCompOpSet);
  layer_add_child(window_get_root_layer(window), (Layer*)second_hand_layer);
  rot_bitmap_layer_set_angle(second_hand_layer, 0);
}

static void main_window_unload(Window * window) {
  // destroy bitmap layer
  rot_bitmap_layer_destroy(second_hand_layer);
  rot_bitmap_layer_destroy(minute_hand_inner_layer);
  rot_bitmap_layer_destroy(minute_hand_outer_layer);
  rot_bitmap_layer_destroy(hour_hand_inner_layer);
  rot_bitmap_layer_destroy(hour_hand_outer_layer);

  // destroy bitmap
  gbitmap_destroy(second_hand_bitmap);
  gbitmap_destroy(minute_hand_inner_bitmap);
  gbitmap_destroy(minute_hand_outer_bitmap);
  gbitmap_destroy(hour_hand_inner_bitmap);
  gbitmap_destroy(hour_hand_outer_bitmap);

  // destroy text layer
  text_layer_destroy(s_date_layer);

  // destroy background
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // show the window on the watch with animated = true
  window_stack_push(s_main_window, true);

  // register callbacks
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_register_outbox_sent(outbox_sent_handler);

  // display correct time right from the start
  update_time();

  // register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, rotate_handler);

  // open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

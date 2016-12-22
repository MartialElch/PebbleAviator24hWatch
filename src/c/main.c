#include <pebble.h>

static Window *s_main_window;
static Layer *s_main_layer;
static TextLayer *s_date_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static bool display_seconds;
static bool invert;

static GPath *s_sec_hand_path_ptr = NULL;
static GPath *s_min_hand_path_ptr = NULL;
static GPath *s_hour_hand_path_ptr = NULL;

#ifdef PBL_COLOR
  #define SEC_HAND_COLOR         GColorRajah
  #define SEC_HAND_INVERT_COLOR  GColorRed
  #define MINHOUR_HAND_COLOR     GColorMintGreen
#else
  #define SEC_HAND_COLOR         GColorWhite
  #define SEC_HAND_INVERT_COLOR  GColorBlack
  #define MINHOUR_HAND_COLOR     GColorWhite
#endif

static const GPathInfo SEC_HAND_PATH = {
  2,
  (GPoint []) {
    {0, -4}, {0, -79},
  }
};

static const GPathInfo MIN_HAND_PATH = {
  7,
  (GPoint []) {
    {-1, 0}, {-1, -13}, {-5, -50}, {0, -70}, {5, -50}, {1, -13}, {1, 0}
  }
};

static const GPathInfo HOUR_HAND_PATH = {
  7,
  (GPoint []) {
    {-3, 0}, {-3, -13}, {-7, -42}, {0, -52}, {7, -42}, {3, -13}, {3, 0}
  }
};

enum {
  KEY_SECONDS = 0,
  KEY_INVERT = 1
};

static void update_display(Layer *s_main_layer, GContext* ctx) {
  // Fill the path:
  graphics_context_set_fill_color(ctx, MINHOUR_HAND_COLOR);
  gpath_draw_filled(ctx, s_hour_hand_path_ptr);
  gpath_draw_filled(ctx, s_min_hand_path_ptr);

  // draw circle in middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, GPoint(71,83 ), 8);

  // Stroke the path:
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_outline(ctx, s_hour_hand_path_ptr);
  gpath_draw_outline(ctx, s_min_hand_path_ptr);
  if (display_seconds) {
    if (!invert) {
      graphics_context_set_stroke_color(ctx, SEC_HAND_COLOR);      
    } else {
      graphics_context_set_stroke_color(ctx, SEC_HAND_INVERT_COLOR);           
    }
    gpath_draw_outline(ctx, s_sec_hand_path_ptr);
  }
}

static void update_time() {
  // date buffer
  static char buffer[] = "00";
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  if (display_seconds) {
    // update seconds
    gpath_rotate_to(s_sec_hand_path_ptr,  (tick_time->tm_sec * TRIG_MAX_ANGLE / 60));
  }
  // update minutes
  gpath_rotate_to(s_min_hand_path_ptr,  ((tick_time->tm_min + tick_time->tm_sec/60.0) * TRIG_MAX_ANGLE / 60));
  // update hours
  gpath_rotate_to(s_hour_hand_path_ptr, ((tick_time->tm_hour + tick_time->tm_min/60.0) * TRIG_MAX_ANGLE / 24));

  // update date
  snprintf(buffer, sizeof("00"), "%d", tick_time->tm_mday);
  text_layer_set_text(s_date_layer, buffer);

  // update display
  layer_mark_dirty(s_main_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // date buffer
  static char buffer[] = "00";

  if (display_seconds) {
    // update seconds
    gpath_rotate_to(s_sec_hand_path_ptr,  (tick_time->tm_sec * TRIG_MAX_ANGLE / 60));
  }

  // update minute handle only every 10 sec
  if (tick_time->tm_sec%10 == 0) {
    // update minutes
    gpath_rotate_to(s_min_hand_path_ptr,  ((tick_time->tm_min + tick_time->tm_sec/60.0) * TRIG_MAX_ANGLE / 60));
  }

  // update hour handle only every 10 min
  if (units_changed & MINUTE_UNIT) {
    if (tick_time->tm_min%10 == 0) {
      // update hours
      gpath_rotate_to(s_hour_hand_path_ptr, ((tick_time->tm_hour + tick_time->tm_min/60.0) * TRIG_MAX_ANGLE / 24));
    }
  }

  // update date only when it changes
  if (units_changed & DAY_UNIT) {
    snprintf(buffer, sizeof("00"), "%d", tick_time->tm_mday);
    text_layer_set_text(s_date_layer, buffer);
  }

  // update display
  layer_mark_dirty(s_main_layer);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);

  // read persistent data
  display_seconds = true;
  if (persist_exists(KEY_SECONDS)) {
    display_seconds = persist_read_bool(KEY_SECONDS);
  }
  invert = false;
  if (persist_exists(KEY_INVERT)) {
    invert = persist_read_bool(KEY_INVERT);
  }

  // Create GBitmap, then set to created BitmapLayer
  if (invert) {
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_INVERT);
  } else {
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  }
  s_background_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));

  // create text layer for date display
  s_date_layer = text_layer_create(GRect(115, 74, 12, 14));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

  // create display canvas
  s_main_layer = layer_create(bounds);

  // register callback to execute when layer ist marked dirty
  layer_set_update_proc(s_main_layer, update_display);
  layer_add_child(window_get_root_layer(window), s_main_layer);

  // setup watch hands
  // create watch hands with paths
  s_sec_hand_path_ptr = gpath_create(&SEC_HAND_PATH);
  s_min_hand_path_ptr = gpath_create(&MIN_HAND_PATH);
  s_hour_hand_path_ptr = gpath_create(&HOUR_HAND_PATH);

  // Translate to middle of screen:
  gpath_move_to(s_sec_hand_path_ptr, GPoint(71, 83));
  gpath_move_to(s_min_hand_path_ptr, GPoint(71, 83));
  gpath_move_to(s_hour_hand_path_ptr, GPoint(71, 83));
}

static void main_window_unload(Window * window) {
  // destroy text layer
  text_layer_destroy(s_date_layer);

  // destroy watch hands layer
  layer_destroy(s_main_layer);

  // destroy background
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);
}

static void reconfigure() {
  main_window_unload(s_main_window);
  main_window_load(s_main_window);
  update_time();
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
          persist_write_bool(KEY_SECONDS, true);
          display_seconds = true;
          // update display
          update_time();
        } else if (strcmp(t->value->cstring, "off") == 0) {
          persist_write_bool(KEY_SECONDS, false);
          display_seconds = false;
        }
        break;
      case KEY_INVERT:
        if (strcmp(t->value->cstring, "on") == 0) {
          persist_write_bool(KEY_INVERT, true);
          invert = true;
          reconfigure();
        } else if (strcmp(t->value->cstring, "off") == 0) {
          persist_write_bool(KEY_INVERT, false);
          invert = false;
          reconfigure();
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

static void init() {
  s_main_window = window_create();
  
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
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

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

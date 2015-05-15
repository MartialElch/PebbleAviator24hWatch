#include <pebble.h>

static Window *s_main_window;
static Layer *s_main_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static GPath *s_sec_hand_path_ptr = NULL;
static GPath *s_min_hand_path_ptr = NULL;
static GPath *s_hour_hand_path_ptr = NULL;

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

static void update_display(Layer *s_main_layer, GContext* ctx) {
  // Fill the path:
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, s_hour_hand_path_ptr);
  gpath_draw_filled(ctx, s_min_hand_path_ptr);

  // draw circle in middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, GPoint(71,83 ), 8);

  // Stroke the path:
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_outline(ctx, s_hour_hand_path_ptr);
  gpath_draw_outline(ctx, s_min_hand_path_ptr);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  gpath_draw_outline(ctx, s_sec_hand_path_ptr);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // update seconds
  gpath_rotate_to(s_sec_hand_path_ptr,  (tick_time->tm_sec * TRIG_MAX_ANGLE / 60));
  // update minutes
  gpath_rotate_to(s_min_hand_path_ptr,  ((tick_time->tm_min + tick_time->tm_sec/60.0) * TRIG_MAX_ANGLE / 60));
  // update hours
  gpath_rotate_to(s_hour_hand_path_ptr, ((tick_time->tm_hour + tick_time->tm_min/60.0 + 12) * TRIG_MAX_ANGLE / 24));

  // update display
  layer_mark_dirty(s_main_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // update seconds
  gpath_rotate_to(s_sec_hand_path_ptr,  (tick_time->tm_sec * TRIG_MAX_ANGLE / 60));
  
  // update minute handle only every 10 sec
  if (tick_time->tm_sec%10 == 0) {
    // update minutes
    gpath_rotate_to(s_min_hand_path_ptr,  ((tick_time->tm_min + tick_time->tm_sec/60.0) * TRIG_MAX_ANGLE / 60));
  }

  // update hour handle only every 10 min
  if (units_changed & MINUTE_UNIT) {
    if (tick_time->tm_min%10 == 0) {
      // update hours
      gpath_rotate_to(s_hour_hand_path_ptr, ((tick_time->tm_hour + tick_time->tm_min/60.0 + 12) * TRIG_MAX_ANGLE / 24));
    }
  }

  // update display
  layer_mark_dirty(s_main_layer);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  s_background_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));

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
  // destroy watch hands layer
  layer_destroy(s_main_layer);

  // destroy background
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);
}

static void init() {
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // show the window on the watch with animated = true
  window_stack_push(s_main_window, true);

  // display correct time right from the start
  update_time();

  // register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

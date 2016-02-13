#include <pebble.h>

#define NUM_MENU_SECTIONS 2
#define NUM_MENU_ICONS 3
#define NUM_FIRST_MENU_ITEMS 3
#define NUM_SECOND_MENU_ITEMS 1

static Window *window;
static TextLayer *text_layer;

static MenuLayer *menu_layer;

static GBitmap *menu_icons[NUM_MENU_ICONS];

static int current_icon = 0;

// You can draw arbitrary things in a menu item such as a background
static GBitmap *menu_background;

// A callback is used to specify the amount of sections of menu items
// With this, you can dynamically add and remove sections
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return NUM_MENU_SECTIONS;
}

// Each section has a number of items;  we use a callback to specify this
// You can also dynamically add and remove items using this
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    switch (section_index) {
        case 0:
            return NUM_FIRST_MENU_ITEMS;
            
        case 1:
            return NUM_SECOND_MENU_ITEMS;
            
        default:
            return 0;
    }
}

// A callback is used to specify the height of the section header
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    // This is a define provided in pebble.h that you may use for the default height
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

// Here we draw what each header is
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    // Determine which section we're working with
    switch (section_index) {
        case 0:
            // Draw title text in the section header
            menu_cell_basic_header_draw(ctx, cell_layer, "Some example items");
            break;
            
        case 1:
            menu_cell_basic_header_draw(ctx, cell_layer, "One more");
            break;
    }
}

// This is the menu item draw callback where you specify what each item should look like
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    // Determine which section we're going to draw in
    switch (cell_index->section) {
        case 0:
            // Use the row to specify which item we'll draw
            switch (cell_index->row) {
                case 0:
                    // This is a basic menu item with a title and subtitle
                    menu_cell_basic_draw(ctx, cell_layer, "Basic Item", "With a subtitle", NULL);
                    break;
                    
                case 1:
                    // This is a basic menu icon with a cycling icon
                    menu_cell_basic_draw(ctx, cell_layer, "Icon Item", "Select to cycle", menu_icons[current_icon]);
                    break;
                    
                case 2:
                    // Here we use the graphics context to draw something different
                    // In this case, we show a strip of a watchface's background
                    graphics_draw_bitmap_in_rect(ctx, menu_background,
                                                 (GRect){ .origin = GPointZero, .size = layer_get_frame((Layer*) cell_layer).size });
                    break;
            }
            break;
            
        case 1:
            switch (cell_index->row) {
                case 0:
                    // There is title draw for something more simple than a basic menu item
                    menu_cell_title_draw(ctx, cell_layer, "Final Item");
                    break;
            }
    }
}

// Here we capture when a user selects a menu item
void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    // Use the row to specify which item will receive the select action
    switch (cell_index->row) {
            // This is the menu item with the cycling icon
        case 1:
            // Cycle the icon
            current_icon = (current_icon + 1) % NUM_MENU_ICONS;
            // After changing the icon, mark the layer to have it updated
            layer_mark_dirty(menu_layer_get_layer(menu_layer));
            break;
    }
    
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Down");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
    // Here we load the bitmap assets
    // resource_init_current_app must be called before all asset loading
    int num_menu_icons = 0;
    menu_icons[num_menu_icons++] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MENU_ICON_BIG_WATCH);
    menu_icons[num_menu_icons++] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MENU_ICON_SECTOR_WATCH);
    menu_icons[num_menu_icons++] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MENU_ICON_BINARY_WATCH);
    
    // And also load the background
    menu_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_BRAINS);
    
    // Now we prepare to initialize the menu layer
    // We need the bounds to specify the menu layer's viewport size
    // In this case, it'll be the same as the window's
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

    // Create the menu layer
    menu_layer = menu_layer_create(bounds);
    
    // Set all the callbacks for the menu layer
    menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks){
        .get_num_sections = menu_get_num_sections_callback,
        .get_num_rows = menu_get_num_rows_callback,
        .get_header_height = menu_get_header_height_callback,
        .draw_header = menu_draw_header_callback,
        .draw_row = menu_draw_row_callback,
        .select_click = menu_select_callback,
    });

    // Bind the menu layer's click config provider to the window for interactivity
    menu_layer_set_click_config_onto_window(menu_layer, window);
    
    // Add it to the window for display
    layer_add_child(window_layer, menu_layer_get_layer(menu_layer));

    
 // text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });
  //text_layer_set_text(text_layer, "Press a button");
  //text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  //layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
    // Destroy the menu layer
    menu_layer_destroy(menu_layer);
    
    // Cleanup the menu icons
    for (int i = 0; i < NUM_MENU_ICONS; i++) {
        gbitmap_destroy(menu_icons[i]);
    }
    
    // And cleanup the background
    gbitmap_destroy(menu_background);

    text_layer_destroy(text_layer);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}

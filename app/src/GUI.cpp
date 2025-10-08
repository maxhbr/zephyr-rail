#include "GUI.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gui, LOG_LEVEL_INF);

// Conditional compilation for LVGL availability
#ifdef CONFIG_LVGL
#define GUI_AVAILABLE 1
#else
#define GUI_AVAILABLE 0
#endif

// Constructor
GUI::GUI()
    : display_dev(nullptr), main_screen(nullptr), tabview(nullptr),
      status_bar(nullptr), move_tab(nullptr), stack_tab(nullptr),
      config_tab(nullptr), status_tab(nullptr), status_label(nullptr),
      status_label_left(nullptr), status_label_right(nullptr),
      position_label(nullptr), target_label(nullptr), step_size_roller(nullptr),
      move_forward_btn(nullptr), move_backward_btn(nullptr), home_btn(nullptr),
      stack_plan_label(nullptr), step_number_roller(nullptr),
      stack_start_btn(nullptr), stack_stop_btn(nullptr), config_table(nullptr),
      status_table(nullptr) {
#ifdef CONFIG_LVGL
  font_title = &lv_font_montserrat_14;
  font_normal = &lv_font_montserrat_14;
  font_small = &lv_font_montserrat_14;
#else
  font_title = nullptr;
  font_normal = nullptr;
  font_small = nullptr;
#endif
}

// Destructor
GUI::~GUI() { deinit(); }

// Initialize the GUI
bool GUI::init() {
  LOG_INF("Initializing GUI");

#if !GUI_AVAILABLE
  LOG_WRN("LVGL not available - GUI disabled");
  return false;
#endif

#ifdef CONFIG_LVGL
  // Get display device
  display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!device_is_ready(display_dev)) {
    LOG_ERR("Display device not ready");
    return false;
  }

  // Initialize LVGL components
  init_display();
  init_styles();
  init_main_screen();
  init_tabview();
  init_status_bar();

  // Setup tabs
  setup_move_tab();
  setup_stack_tab();
  setup_config_tab();
  setup_status_tab();

  LOG_INF("GUI initialized successfully");
#endif
  return true;
}

// Deinitialize the GUI
void GUI::deinit() {
#ifdef CONFIG_LVGL
  if (main_screen) {
    lv_obj_del(main_screen);
    main_screen = nullptr;
  }
#endif
}

// Initialize display
void GUI::init_display() {
  LOG_INF("Initializing display");
  // Display initialization is typically handled by Zephyr's display subsystem
}

// Initialize styles
void GUI::init_styles() {
#ifdef CONFIG_LVGL
  // Initialize title style
  lv_style_init(&style_title);
  lv_style_set_text_font(&style_title, font_title);
  lv_style_set_text_color(&style_title, lv_color_white());

  // Initialize button style
  lv_style_init(&style_button);
  lv_style_set_radius(&style_button, 5);
  lv_style_set_bg_color(&style_button, lv_color_hex(0x2196F3));
  lv_style_set_bg_grad_color(&style_button, lv_color_hex(0x1976D2));
  lv_style_set_bg_grad_dir(&style_button, LV_GRAD_DIR_VER);

  // Initialize panel style
  lv_style_init(&style_panel);
  lv_style_set_radius(&style_panel, 10);
  lv_style_set_bg_color(&style_panel, lv_color_hex(0x424242));
  lv_style_set_border_width(&style_panel, 1);
  lv_style_set_border_color(&style_panel, lv_color_hex(0x757575));
#endif
}

// Initialize main screen
void GUI::init_main_screen() {
  main_screen = lv_scr_act();
  lv_obj_set_style_bg_color(main_screen, lv_color_black(), 0);
}

// Initialize tabview
void GUI::init_tabview() {
  tabview = lv_tabview_create(main_screen);
  lv_obj_set_size(tabview, LV_PCT(100), LV_PCT(85));
  lv_obj_align(tabview, LV_ALIGN_TOP_MID, 0, 40);

  // Create tabs
  move_tab = lv_tabview_add_tab(tabview, "Move");
  stack_tab = lv_tabview_add_tab(tabview, "Stack");
  config_tab = lv_tabview_add_tab(tabview, "Config");
  status_tab = lv_tabview_add_tab(tabview, "Status");
}

// Initialize status bar
void GUI::init_status_bar() {
  // Main status container
  lv_obj_t *status_container = lv_obj_create(main_screen);
  lv_obj_set_size(status_container, LV_PCT(100), 35);
  lv_obj_align(status_container, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(status_container, lv_color_hex(0x212121), 0);
  lv_obj_set_style_border_width(status_container, 0, 0);
  lv_obj_set_style_radius(status_container, 0, 0);

  // Left status label
  status_label_left = lv_label_create(status_container);
  lv_label_set_text(status_label_left, "Ready");
  lv_obj_align(status_label_left, LV_ALIGN_LEFT_MID, 10, 0);
  lv_obj_set_style_text_color(status_label_left, lv_color_white(), 0);

  // Center status label
  status_label = lv_label_create(status_container);
  lv_label_set_text(status_label, "Zephyr Rail");
  lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
  lv_obj_add_style(status_label, &style_title, 0);

  // Right status label
  status_label_right = lv_label_create(status_container);
  lv_label_set_text(status_label_right, "00:00");
  lv_obj_align(status_label_right, LV_ALIGN_RIGHT_MID, -10, 0);
  lv_obj_set_style_text_color(status_label_right, lv_color_white(), 0);
}

// Setup move tab
void GUI::setup_move_tab() {
  // Position display
  lv_obj_t *pos_panel = lv_obj_create(move_tab);
  lv_obj_set_size(pos_panel, LV_PCT(100), 80);
  lv_obj_align(pos_panel, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_add_style(pos_panel, &style_panel, 0);

  position_label = create_label(pos_panel, "Position: 0 mm");
  lv_obj_align(position_label, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_add_style(position_label, &style_title, 0);

  target_label = create_label(pos_panel, "Target: 0 mm");
  lv_obj_align(target_label, LV_ALIGN_TOP_MID, 0, 40);

  // Step size roller
  step_size_roller = create_roller(pos_panel, "0.1mm\\n1mm\\n10mm\\n100mm");
  lv_obj_align(step_size_roller, LV_ALIGN_CENTER, 0, 0);

  // Control buttons
  lv_obj_t *btn_panel = lv_obj_create(move_tab);
  lv_obj_set_size(btn_panel, LV_PCT(100), 100);
  lv_obj_align(btn_panel, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_add_style(btn_panel, &style_panel, 0);

  move_backward_btn = create_button(btn_panel, "<<", move_backward_event_cb);
  lv_obj_align(move_backward_btn, LV_ALIGN_LEFT_MID, 10, 0);

  home_btn = create_button(btn_panel, "HOME", home_event_cb);
  lv_obj_align(home_btn, LV_ALIGN_CENTER, 0, 0);

  move_forward_btn = create_button(btn_panel, ">>", move_forward_event_cb);
  lv_obj_align(move_forward_btn, LV_ALIGN_RIGHT_MID, -10, 0);
}

// Setup stack tab
void GUI::setup_stack_tab() {
  // Stack plan display
  stack_plan_label = create_label(stack_tab, "Stack Plan: Not loaded");
  lv_obj_align(stack_plan_label, LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_add_style(stack_plan_label, &style_title, 0);

  // Step number selection
  step_number_roller = create_roller(stack_tab, "10\\n20\\n50\\n100\\n200");
  lv_obj_align(step_number_roller, LV_ALIGN_CENTER, 0, -20);

  // Control buttons
  stack_start_btn = create_button(stack_tab, "START", stack_start_event_cb);
  lv_obj_align(stack_start_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);

  stack_stop_btn = create_button(stack_tab, "STOP", stack_stop_event_cb);
  lv_obj_align(stack_stop_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
}

// Setup config tab
void GUI::setup_config_tab() {
  config_table = create_table(config_tab, 5, 2);
  lv_obj_align(config_table, LV_ALIGN_CENTER, 0, 0);

  // Set table headers
  set_table_cell_value(config_table, 0, 0, "Parameter");
  set_table_cell_value(config_table, 0, 1, "Value");

  // Add some default config items
  set_table_cell_value(config_table, 1, 0, "Steps/mm");
  set_table_cell_value_int(config_table, 1, 1, 200);

  set_table_cell_value(config_table, 2, 0, "Max Speed");
  set_table_cell_value_int(config_table, 2, 1, 1000);

  set_table_cell_value(config_table, 3, 0, "Acceleration");
  set_table_cell_value_int(config_table, 3, 1, 500);

  set_table_cell_value(config_table, 4, 0, "Backlash");
  set_table_cell_value(config_table, 4, 1, "0.1mm");
}

// Setup status tab
void GUI::setup_status_tab() {
  status_table = create_table(status_tab, 6, 2);
  lv_obj_align(status_table, LV_ALIGN_CENTER, 0, 0);

  // Set table headers
  set_table_cell_value(status_table, 0, 0, "Item");
  set_table_cell_value(status_table, 0, 1, "Status");

  // Add status items
  set_table_cell_value(status_table, 1, 0, "System");
  set_table_cell_value(status_table, 1, 1, "Ready");

  set_table_cell_value(status_table, 2, 0, "Stepper");
  set_table_cell_value(status_table, 2, 1, "Idle");

  set_table_cell_value(status_table, 3, 0, "Position");
  set_table_cell_value_int(status_table, 3, 1, 0);

  set_table_cell_value(status_table, 4, 0, "Stack");
  set_table_cell_value(status_table, 4, 1, "Empty");

  set_table_cell_value(status_table, 5, 0, "Bluetooth");
  set_table_cell_value(status_table, 5, 1, "Disconnected");
}

// Utility method to create buttons
lv_obj_t *GUI::create_button(lv_obj_t *parent, const char *text,
                             lv_event_cb_t event_cb) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 80, 40);
  lv_obj_add_style(btn, &style_button, 0);

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  if (event_cb) {
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, this);
  }

  return btn;
}

// Utility method to create labels
lv_obj_t *GUI::create_label(lv_obj_t *parent, const char *text) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  return label;
}

// Utility method to create rollers
lv_obj_t *GUI::create_roller(lv_obj_t *parent, const char *options) {
  lv_obj_t *roller = lv_roller_create(parent);
  lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);
  lv_obj_set_size(roller, 120, 80);
  return roller;
}

// Utility method to create tables
lv_obj_t *GUI::create_table(lv_obj_t *parent, int rows, int cols) {
  lv_obj_t *table = lv_table_create(parent);
  lv_table_set_row_cnt(table, rows);
  lv_table_set_col_cnt(table, cols);
  lv_obj_set_size(table, LV_PCT(90), LV_PCT(80));

  // Set column widths
  for (int i = 0; i < cols; i++) {
    lv_table_set_col_width(table, i, 150);
  }

  return table;
}

// Event callback for move forward button
void GUI::move_forward_event_cb(lv_event_t *e) {
  GUI *gui = (GUI *)lv_event_get_user_data(e);
  LOG_INF("Move forward button pressed");
  // Add actual movement logic here
}

// Event callback for move backward button
void GUI::move_backward_event_cb(lv_event_t *e) {
  GUI *gui = (GUI *)lv_event_get_user_data(e);
  LOG_INF("Move backward button pressed");
  // Add actual movement logic here
}

// Event callback for home button
void GUI::home_event_cb(lv_event_t *e) {
  GUI *gui = (GUI *)lv_event_get_user_data(e);
  LOG_INF("Home button pressed");
  // Add actual homing logic here
}

// Event callback for stack start button
void GUI::stack_start_event_cb(lv_event_t *e) {
  GUI *gui = (GUI *)lv_event_get_user_data(e);
  LOG_INF("Stack start button pressed");
  // Add actual stack start logic here
}

// Event callback for stack stop button
void GUI::stack_stop_event_cb(lv_event_t *e) {
  GUI *gui = (GUI *)lv_event_get_user_data(e);
  LOG_INF("Stack stop button pressed");
  // Add actual stack stop logic here
}

// Update the GUI with current status
void GUI::update(const struct stepper_with_target_status *stepper_status,
                 const struct stack_status *stack_status) {
  if (!stepper_status || !stack_status) {
    return;
  }

  // Update position display
  if (position_label) {
    char pos_text[64];
    // Convert position using the helper method
    int pos_nm = position_as_nm(stepper_status->stepper_status.pitch_per_rev,
                                stepper_status->stepper_status.pulses_per_rev,
                                stepper_status->stepper_status.position);
    snprintf(pos_text, sizeof(pos_text), "Position: %.2f mm",
             pos_nm / 1000000.0);
    lv_label_set_text(position_label, pos_text);
  }

  // Update target display
  if (target_label) {
    char target_text[64];
    int target_nm =
        position_as_nm(stepper_status->stepper_status.pitch_per_rev,
                       stepper_status->stepper_status.pulses_per_rev,
                       stepper_status->target_position);
    snprintf(target_text, sizeof(target_text), "Target: %.2f mm",
             target_nm / 1000000.0);
    lv_label_set_text(target_label, target_text);
  }

  // Update stack plan
  if (stack_plan_label) {
    char stack_text[64];
    if (stack_status->index_in_stack.has_value()) {
      snprintf(stack_text, sizeof(stack_text), "Stack: %d/%d steps",
               stack_status->index_in_stack.value(),
               stack_status->length_of_stack);
    } else {
      snprintf(stack_text, sizeof(stack_text), "Stack: %d steps total",
               stack_status->length_of_stack);
    }
    lv_label_set_text(stack_plan_label, stack_text);
  }

  // Update status table
  if (status_table) {
    int pos_nm = position_as_nm(stepper_status->stepper_status.pitch_per_rev,
                                stepper_status->stepper_status.pulses_per_rev,
                                stepper_status->stepper_status.position);
    char pos_str[32];
    snprintf(pos_str, sizeof(pos_str), "%.2f mm", pos_nm / 1000000.0);
    set_table_cell_value(status_table, 3, 1, pos_str);

    const char *stepper_state = stepper_status->is_moving ? "Moving" : "Idle";
    set_table_cell_value(status_table, 2, 1, stepper_state);

    const char *stack_state =
        stack_status->index_in_stack.has_value() ? "Active" : "Idle";
    set_table_cell_value(status_table, 4, 1, stack_state);
  }
}

// Set main status text
void GUI::set_status(const char *status) {
  if (status_label && status) {
    lv_label_set_text(status_label, status);
  }
}

// Set left status text
void GUI::set_status_left(const char *status_left) {
  if (status_label_left && status_left) {
    lv_label_set_text(status_label_left, status_left);
  }
}

// Set right status text
void GUI::set_status_right(const char *status_right) {
  if (status_label_right && status_right) {
    lv_label_set_text(status_label_right, status_right);
  }
}

// Set brightness
void GUI::set_brightness(uint8_t brightness) {
  if (display_dev) {
    // Implementation depends on display driver capabilities
    LOG_INF("Setting brightness to %d", brightness);
  }
}

// Set backlight
void GUI::set_backlight(bool on) {
  if (display_dev) {
    // Implementation depends on display driver capabilities
    LOG_INF("Setting backlight: %s", on ? "ON" : "OFF");
  }
}

// Run LVGL task handler
void GUI::run_task_handler() { lv_task_handler(); }

// Show specific tab
void GUI::show_tab(int tab_index) {
  if (tabview) {
    lv_tabview_set_act(tabview, tab_index, false);
  }
}

// Set header visibility
void GUI::set_header_visible(bool visible) {
  if (status_bar) {
    if (visible) {
      lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(status_bar, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

// Set table cell value
void GUI::set_table_cell_value(lv_obj_t *table, int row, int col,
                               const char *value) {
  if (table && value) {
    lv_table_set_cell_value(table, row, col, value);
  }
}

// Set table cell integer value
void GUI::set_table_cell_value_int(lv_obj_t *table, int row, int col,
                                   int value) {
  if (table) {
    char text[32];
    snprintf(text, sizeof(text), "%d", value);
    lv_table_set_cell_value(table, row, col, text);
  }
}

// Helper method for position conversion
int GUI::position_as_nm(int pitch_per_rev, int pulses_per_rev, int position) {
  // Convert stepper position to nanometers
  // This is a simplified calculation - adjust based on your specific setup
  return (position * pitch_per_rev * 1000000) / pulses_per_rev;
}
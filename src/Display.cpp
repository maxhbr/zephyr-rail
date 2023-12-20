#include "Display.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display);

void Display::init_styles()
{
  lv_style_init(&style_normal);
  lv_style_set_text_font(&style_normal, font_normal);

  lv_style_init(&style_button);
#if 0
  lv_style_set_bg_color(&style_button, LV_STATE_DEFAULT, LV_COLOR_GRAY);
  lv_style_set_bg_color(&style_button, LV_STATE_PRESSED, LV_COLOR_RED);
  lv_style_set_radius(&style_button, LV_STATE_DEFAULT, 5);
  lv_style_set_bg_opa(&style_button, LV_STATE_DEFAULT, LV_OPA_COVER);
#endif

  lv_style_init(&style_box);
#if 0
  lv_style_set_bg_color(&style_box, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_style_set_bg_opa(&style_box, LV_STATE_DEFAULT, LV_OPA_COVER);
#endif
}

void Display::init_header(lv_obj_t *parent)
{
  header = add_container(parent, LV_HOR_RES, 25);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
}

void Display::init_tabview(lv_obj_t *parent)
{
  tabview = lv_tabview_create(parent, LV_DIR_TOP, 40);
  lv_obj_set_size(tabview, LV_HOR_RES, LV_VER_RES - 40);
  // lv_obj_add_style(tabview, 0, &style_normal);
  // lv_obj_align(tabview, NULL, LV_ALIGN_BOTTOM_MID, 0, 0);
}

Display::Display(const struct zbus_channel *_status_chan) : status_chan{_status_chan}
{
  if (!device_is_ready(display_dev))
  {
    LOG_ERR("Device not ready, aborting");
    return;
  }
  LOG_INF("initialize display...");
  init_styles();
  init_tabview(lv_scr_act());
  // init_header(lv_scr_act());

  lv_obj_t *status_tab = make_tab("status");

  status_label = lv_label_create(status_tab);
  lv_label_set_text(status_label, "$status");
  lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);

  debug_label = lv_label_create(status_tab);
  lv_label_set_text(debug_label, "$debug");
  lv_obj_align(debug_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

  lv_obj_t *move_tab = make_tab("move");
  lv_obj_t *stack_tab = make_tab("stack");
  lv_obj_t *config_tab = make_tab("cfg");

  // lv_indev_set_group(lvgl_input_get_indev(lvgl_keypad), lv_group_get_default());

  lv_task_handler();
  display_blanking_off(display_dev);
  lv_task_handler();

  LOG_INF("...initialize display done");
}

void Display::update_status()
{
  char buf[1000];
  const struct model_status *status = (const struct model_status *)zbus_chan_const_msg(status_chan);
  if (status != NULL)
  {
    const struct stepper_with_target_status *stepper_with_target_status = &status->stepper_with_target_status;
    const struct stepper_status *stepper_status = &stepper_with_target_status->stepper_status;

    snprintf(buf, sizeof(buf), "%d >>> %d >>> %d\ntarget: %d\nis_moving: %d",
             status->lower_bound, stepper_status->position, status->upper_bound,
             stepper_with_target_status->target_position,
             stepper_with_target_status->is_moving);
    lv_label_set_text(status_label, buf);
  }
}

void Display::set_debug_text(const char *text)
{
  lv_label_set_text(debug_label, text);
}

#if 0
lv_obj_t *Display::get_header() { return header; }
#endif

lv_obj_t *Display::make_tab(const char *title)
{
  return lv_tabview_add_tab(tabview, title);
}

lv_obj_t *Display::add_container(lv_obj_t *parent, int width, int height)
{
  lv_obj_t *container = lv_win_create(parent, NULL);
  lv_obj_set_size(container, width, height);
  // lv_cont_set_fit(container, LV_FIT_TIGHT);
  // lv_cont_set_layout(container, LV_LAYOUT_COLUMN_MID);
  return container;
}

#if 0
lv_obj_t *Display::add_label(lv_obj_t *parent) {
  lv_obj_t *lbl = lv_label_create(parent, NULL);
  lv_obj_add_style(lbl, 0, &style_normal);
  return lbl;
}

lv_obj_t *Display::add_panel(lv_obj_t *parent) {
  lv_obj_t *panel = lv_obj_create(parent, NULL);
  lv_obj_add_style(panel, 0, &style_box);
  return panel;
}

lv_obj_t *Display::add_button(lv_obj_t *parent, const char *label_text,
                              int width, int height) {
  lv_obj_t *btn = lv_btn_create(parent, NULL);
  lv_obj_set_height(btn, height);
  lv_obj_set_width(btn, width);
  lv_obj_add_style(btn, 0, &style_button);
  if (label_text != NULL) {
    lv_obj_t *label = lv_label_create(btn, NULL);
    lv_label_set_text(label, label_text);
    lv_obj_add_style(label, 0, &style_normal);
  }
  return btn;
}

lv_obj_t *Display::add_roller(lv_obj_t *parent, const char *options) {
  lv_obj_t *roller = lv_roller_create(parent, NULL);
  lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);

  lv_roller_set_visible_row_count(roller, 4);
  return roller;
}

void Display::set_header_visible(bool is_visible) {
  lv_obj_set_hidden(header, !is_visible);
}
#endif

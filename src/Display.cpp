#include "Display.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display);

void Display::init_header(lv_obj_t *parent)
{
  header = add_container(parent, LV_HOR_RES, 25);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
}

void Display::init_tabview(lv_obj_t *parent)
{
  tabview = lv_tabview_create(parent, LV_DIR_TOP, 40);
  lv_obj_set_size(tabview, LV_HOR_RES, LV_VER_RES - 40);
}

Display::Display()
{
  k_mutex_init(&lvgl_mutex);
  if (!device_is_ready(display_dev))
  {
    LOG_ERR("Device not ready, aborting");
    return;
  }
  LOG_INF("initialize display...");
  init_tabview(lv_scr_act());

  lv_task_handler();
  display_blanking_off(display_dev);
  lv_task_handler();
  LOG_INF("...initialize display done");
}

void Display::run_task_handler()
{
  lv_task_handler();
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
lv_obj_t *Display::add_label(lv_obj_t *parent)
{
  lv_obj_t *lbl = lv_label_create(parent);
  // lv_obj_add_style(lbl, 0, &style_normal);
  return lbl;
}

lv_obj_t *Display::add_panel(lv_obj_t *parent)
{
  lv_obj_t *panel = lv_obj_create(parent);
  // lv_obj_add_style(panel, 0, &style_box);
  return panel;
}

lv_obj_t *Display::add_button(lv_obj_t *parent, const char *label_text,
                              int width, int height)
{
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_height(btn, height);
  lv_obj_set_width(btn, width);
  // lv_obj_add_style(btn, 0, &style_button);
  if (label_text != NULL)
  {
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, label_text);
    // lv_obj_add_style(label, 0, &style_normal);
  }
  return btn;
}

#if 0
void Display::set_header_visible(bool is_visible) {
  lv_obj_set_hidden(header, !is_visible);
}
#endif

lv_obj_t *Display::add_roller(lv_obj_t *parent, const char *options)
{
  lv_obj_t *roller = lv_roller_create(parent);
  lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);

  lv_roller_set_visible_row_count(roller, 4);
  return roller;
}
#include "Display.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display);

void Display::init_status_labels(lv_obj_t *parent)
{
  status_label = lv_label_create(parent);
  lv_label_set_text(status_label, "$status");
  lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 10);
  status_label_left = lv_label_create(parent);
  lv_label_set_text(status_label_left, "$left");
  lv_obj_align(status_label_left, LV_ALIGN_TOP_LEFT, 0, 0);
  status_label_right = lv_label_create(parent);
  lv_label_set_text(status_label_right, "$right");
  lv_obj_align(status_label_right, LV_ALIGN_TOP_RIGHT, 0, 0);
}

void Display::init_tabview(lv_obj_t *parent)
{
  tabview = lv_tabview_create(parent, LV_DIR_LEFT, 60);
  lv_obj_set_size(tabview, LV_HOR_RES, LV_VER_RES - 30);
  lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, 0);
}

Display::Display()
{
  if (!device_is_ready(display_dev))
  {
    LOG_ERR("Device not ready, aborting");
    return;
  }
  LOG_INF("initialize display...");

  lv_group_t * lv_group = lv_group_create();
  lv_group_set_default(lv_group);
#ifdef CONFIG_LV_Z_KEYPAD_INPUT
	lv_indev_set_group(lvgl_input_get_indev(lvgl_keypad), lv_group);
#endif

  init_status_labels(lv_scr_act());
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

lv_obj_t *Display::make_tab(const char *title)
{
  return lv_tabview_add_tab(tabview, title);
}

lv_obj_t *Display::make_status_table_tab()
{
  lv_obj_t *tab = make_tab("status");
  table = lv_table_create(tab);
  lv_obj_set_width(table, LV_SIZE_CONTENT);
  return tab;
}

void Display::set_table_cell_value(int y, int x, const char *value)
{
  lv_table_set_cell_value(table, y, x, value);
}

void Display::set_table_cell_value_int(int y, int x, int value)
{
  char str[30];
  sprintf(str, "%d", value);
  set_table_cell_value(y,x,str);
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
  if (label_text != NULL)
  {
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, label_text);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  }
  return btn;
}

lv_obj_t *Display::add_roller(lv_obj_t *parent, const char *options)
{
  lv_obj_t *roller = lv_roller_create(parent);
  lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);

  lv_roller_set_visible_row_count(roller, 4);
  return roller;
}

void Display::set_status(const char *status)
{
  lv_label_set_text(status_label, status);
}

void Display::set_status_left(const char *status_left)
{
  lv_label_set_text(status_label_left, status_left);
}

void Display::set_status_right(const char *status_right)
{
  lv_label_set_text(status_label_right, status_right);
}

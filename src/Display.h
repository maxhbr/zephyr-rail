#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/zbus/zbus.h>
#include <lvgl.h>
#include <lvgl_input_device.h>
#include <map>

#include "Model.h"
#include "StepperWithTarget.h"
#include "Stepper.h"

class Display
{
  const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  const struct device *lvgl_keypad = DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_lvgl_keypad_input));

  const struct zbus_channel *status_chan;

  void init_styles();
  lv_obj_t *status_label;
  lv_obj_t *header;
  lv_obj_t *tabview;
  void init_header(lv_obj_t *parent);
  void init_tabview(lv_obj_t *parent);

  const lv_font_t *font_title = &lv_font_montserrat_14;    // _28;
  const lv_font_t *font_subtitle = &lv_font_montserrat_14; // _24;
  const lv_font_t *font_normal = &lv_font_montserrat_14;   // _16;
  const lv_font_t *font_small = &lv_font_montserrat_14;    // _12;

public:
  Display(const struct zbus_channel *_status_chan);

  lv_style_t style_normal;
  lv_style_t style_button;
  lv_style_t style_box;

  // lv_obj_t *get_header();
  lv_obj_t *make_tab(const char *title);
  lv_obj_t *add_container(lv_obj_t *parent, int width, int heigth);
  // lv_obj_t *add_label(lv_obj_t *parent);
  // lv_obj_t *add_panel(lv_obj_t *parent);
  // lv_obj_t *add_button(lv_obj_t *parent, const char *label_text, int width,
  //                      int heigth);
  // lv_obj_t *add_roller(lv_obj_t *parent, const char *options);
  void set_header_visible(bool is_visible);

  void update_status();
};

#endif // __DISPLAY_H_

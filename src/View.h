#ifndef __VIEW_H_
#define __VIEW_H_

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/zephyr.h>
#include <zephyr/types.h>

#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include <map>

#include "Controller.h"
#include "Display.h"
#include "Model.h"
#include "Stepper.h"

class View : public Display {
private:
  Model *model;
  Controller *controller;

  // Devices
  Display *display;

public:
  View(Model *_model, Controller *_controller, Display *_display);
};

#endif // __VIEW_H_
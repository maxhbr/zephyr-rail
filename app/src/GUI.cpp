#include "GUI.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gui, LOG_LEVEL_DBG);

#ifdef CONFIG_DISPLAY

// Constructor
GUI::GUI(const StateMachine *sm) : LoggingGUI(sm) {}

// Static method to initialize the gui
GUI *GUI::initialize_gui(const StateMachine *sm) {
#ifdef CONFIG_DISPLAY
  static GUI gui(sm);
  if (!gui.init()) {
    LOG_ERR("Failed to initialize GUI");
    return nullptr;
  }

  LOG_INF("GUI initialized successfully");
  return &gui;
#else
  return nullptr;
#endif
}

#endif
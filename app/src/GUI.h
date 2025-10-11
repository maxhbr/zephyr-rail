#pragma once

#include "LoggingGUI.h"

class GUI : public LoggingGUI {
public:
  GUI(const StateMachine *sm);
  virtual ~GUI() = default;

  // Static method to initialize GUI
  static GUI *initialize_gui(const StateMachine *sm);
};
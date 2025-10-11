#pragma once

#include "BaseGUI.h"

class LoggingGUI : public BaseGUI {
protected:
  lv_obj_t *log_textarea;

public:
  LoggingGUI(const StateMachine *sm);
  virtual ~LoggingGUI() = default;

  // Override init to add logging functionality
  virtual bool init() override;

  // Override deinit to clean up logging resources
  virtual void deinit() override;

  // Log display method
  void add_log_line(const char *log_data, size_t length);

  // Add log tab to the existing tabview
  void add_log_tab();
};
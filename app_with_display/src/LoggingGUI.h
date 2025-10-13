#pragma once

#include "BaseGUI.h"

class LoggingGUI : public BaseGUI {
protected:
  lv_obj_t *log_textarea;

  void add_log_tab();

public:
  LoggingGUI();
  virtual ~LoggingGUI() = default;

  virtual bool init() override;
  virtual void deinit() override;
  virtual void start() override;
  virtual void run_task_handler() override;

  void add_log_line(const char *log_data, size_t length);
};
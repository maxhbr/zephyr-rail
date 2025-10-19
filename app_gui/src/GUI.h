#pragma once

#include "LoggingGUI.h"

class GUI : public LoggingGUI {
  const StateMachine *sm;

  void add_move_tab();
  void add_stack_tab();

  virtual void
  update_status(const struct stepper_with_target_status *stepper_status,
                const struct stack_status *stack_status);
  int position_as_nm(int pitch_per_rev, int pulses_per_rev, int position);

public:
  GUI(const StateMachine *sm);
  virtual ~GUI() = default;

  virtual bool init() override;
  virtual void deinit() override;
  virtual void start() override;
  virtual void run_task_handler() override;

  static GUI *initialize_gui(const StateMachine *sm);
};
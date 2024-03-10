#pragma once

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/zbus/zbus.h>

#include <optional>

struct stack_status
{
  int lower_bound;
  int upper_bound;
  std::optional<int> index_in_stack;
  int length_of_stack;
};

class Stack
{
  int lower_bound = 0;
  int upper_bound = 0;
  bool start_at_lower = true;

  int expected_length_of_stack = 300;
  int expected_step_size = 30;
  bool compute_via_step_size = false;


  int length_of_stack = 0;
  std::optional<int> index_in_stack = {};
  int *stepps_of_stack = (int *)malloc(sizeof(int) * 2000);

  bool compute_by_step_size(const int start, const int end);
  bool compute_by_expected_length_of_stack(const int start, const int end);
  bool compute();
public:
  Stack();

  void log_state();

  // stacking
  std::optional<int> start_stack();
  std::optional<int> get_current_step();
  void increment_step();
  bool stack_in_progress();

  // configuring stack
  void set_lower_bound(int _lower_bound);
  void set_upper_bound(int _upper_bound);

  const struct stack_status get_status()
  {
    return {
        .lower_bound = lower_bound,
        .upper_bound = upper_bound,
        .index_in_stack = index_in_stack,
        .length_of_stack = length_of_stack,
    };
  }
};

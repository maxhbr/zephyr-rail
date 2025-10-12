#include "Stack.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stack);

Stack::Stack() { LOG_INF("%s", __FUNCTION__); };

void Stack::log_state() { LOG_INF("%i -> %i", lower_bound, upper_bound); };

bool Stack::compute_by_step_size(const int start, const int end) {
  int step_size = expected_step_size;
  if (step_size == 0 || start == end) {
    return;
  }
  if (step_size <= 0 && start < end || step_size >= 0 && end < start) {
    step_size = -step_size;
  }

  length_of_stack = 0;
  stepps_of_stack[length_of_stack] = start;
  while (stepps_of_stack[length_of_stack] <= end) {
    length_of_stack++;
    if (length_of_stack > 1999) {
      return;
    }
    stepps_of_stack[length_of_stack] =
        stepps_of_stack[length_of_stack - 1] + step_size;
  }
  stepps_of_stack[length_of_stack] = end;
  length_of_stack++;
}

bool Stack::compute_by_expected_length_of_stack(const int start,
                                                const int end) {}

bool Stack::compute() {
  int start;
  int end;
  if (start_at_lower) {
    start = lower_bound;
    end = upper_bound;
  } else {
    end = lower_bound;
    start = upper_bound;
  }
  if (compute_via_step_size) {
    return compute_by_step_size(start, end);
  } else {
    return compute_by_expected_length_of_stack(start, end);
  }
}

std::optional<int> Stack::start_stack() {
  bool succ = compute();
  if (!succ) {
    return {};
  }
  index_in_stack = 0;
  return get_current_step();
}

std::optional<int> Stack::get_current_step() {
  if (!index_in_stack.has_value()) {
    return {};
  }
  int actual_index_in_stack = index_in_stack.value();
  if (actual_index_in_stack < 0 || length_of_stack <= actual_index_in_stack) {
    return {};
  }
  return stepps_of_stack[actual_index_in_stack];
}

void Stack::increment_step() {
  if (index_in_stack.has_value()) {
    index_in_stack = index_in_stack.value() + 1;
  }
}

bool Stack::stack_in_progress() {
  return index_in_stack.has_value() && get_current_step().has_value();
}

void Stack::set_lower_bound(int _lower_bound) {
  lower_bound = _lower_bound;
  start_at_lower = true;
}

void Stack::set_upper_bound(int _upper_bound) {
  upper_bound = _upper_bound;
  start_at_lower = false;
}

#include "Stack.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stack, LOG_LEVEL_INF);

Stack::Stack() { LOG_INF("%s", __FUNCTION__); };

char *Stack::state() {
  static char buffer[256];
  if (stack_in_progress()) {
    snprintf(buffer, sizeof(buffer),
             "Stacking in progress: Step %d/%d at position %.3fum < %.3fum < "
             "%.3fum",
             index_in_stack.value() + 1, length_of_stack, nm_as_um(lower_bound),
             nm_as_um(get_current_target().value()), nm_as_um(upper_bound));
  } else {
    double lower_nm = nm_as_um(lower_bound);
    double upper_nm = nm_as_um(upper_bound);
    double diff_nm = upper_nm - lower_nm;
    snprintf(buffer, sizeof(buffer),
             "%.3fum -> %.3fum, diff=%.3fum, start_at=%s", lower_nm, upper_nm,
             diff_nm, start_at_lower ? "lower" : "upper");
  }
  return buffer;
}

void Stack::log_state() { LOG_INF("%s", state()); }

bool Stack::compute_by_step_size(const int start, const int end) {
  int step_size = expected_step_size;
  LOG_DBG("Computing stack from %d to %d with step size %d", start, end,
          step_size);
  if (step_size <= 0 || start == end) {
    LOG_ERR("Invalid step size (=%d) or start (=%d) equals end (=%d)",
            step_size, start, end);
    return false;
  }
  if (end < start) {
    LOG_DBG("Reversing step size for descending stack");
    step_size = -step_size;
  }

  length_of_stack = 0;
  stepps_of_stack[length_of_stack] = start;
  while ((step_size > 0 && stepps_of_stack[length_of_stack] <= end) ||
         (step_size < 0 && stepps_of_stack[length_of_stack] >= end)) {
    length_of_stack++;
    if (length_of_stack > 3999) {
      LOG_ERR("Exceeded maximum stack size of 4000");
      return false;
    }
    stepps_of_stack[length_of_stack] =
        stepps_of_stack[length_of_stack - 1] + step_size;
  }
  stepps_of_stack[length_of_stack] = end;
  length_of_stack++;
  LOG_DBG("Computed %d steps for stack of size %d", length_of_stack, step_size);
  return true;
}

bool Stack::compute_by_expected_length_of_stack(const int start,
                                                const int end) {
  int expected_length = expected_length_of_stack;
  if (expected_length < 2 || start == end) {
    LOG_WRN("Invalid expected length or start equals end");
    return false;
  }
  int step_size = (end - start) / (expected_length - 1);
  if (step_size == 0) {
    LOG_WRN("Computed step size is zero");
    return false;
  }
  stepps_of_stack[0] = start;
  for (int i = 1; i < expected_length; i++) {
    stepps_of_stack[i] = stepps_of_stack[i - 1] + step_size;
  }
  stepps_of_stack[expected_length - 1] = end;
  length_of_stack = expected_length;
  LOG_DBG("Computed %d steps for stack of size %d", length_of_stack, step_size);
  return true;
}

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
  LOG_DBG("Computing stack from %d to %d", start, end);
  if (compute_via_step_size) {
    LOG_DBG("Computing via step size");
    return compute_by_step_size(start, end);
  } else {
    LOG_DBG("Computing via expected length of stack");
    return compute_by_expected_length_of_stack(start, end);
  }
}

std::optional<int> Stack::start_stack() {
  bool succ = compute();
  if (!succ) {
    return {};
  }
  index_in_stack = 0;
  return get_current_target();
}

std::optional<int> Stack::get_current_target() {
  if (!index_in_stack.has_value()) {
    return {};
  }
  int actual_index_in_stack = index_in_stack.value();
  if (actual_index_in_stack < 0 || length_of_stack <= actual_index_in_stack) {
    return {};
  }
  return stepps_of_stack[actual_index_in_stack];
}

std::optional<int> Stack::get_index_in_stack() { return index_in_stack; }

std::optional<int> Stack::get_length_of_stack() {
  if (!index_in_stack.has_value()) {
    return {};
  }
  return length_of_stack;
}

void Stack::increment_target() {
  if (index_in_stack.has_value()) {
    index_in_stack = index_in_stack.value() + 1;
  }
}

bool Stack::stack_in_progress() {
  return index_in_stack.has_value() && get_current_target().has_value();
}

void Stack::set_lower_bound(int _lower_bound) {
  lower_bound = _lower_bound;
  start_at_lower = true;
}

void Stack::set_upper_bound(int _upper_bound) {
  upper_bound = _upper_bound;
  start_at_lower = false;
}

int Stack::get_lower_bound() { return lower_bound; }

int Stack::get_upper_bound() { return upper_bound; }

void Stack::set_expected_length_of_stack(int _expected_length_of_stack) {
  expected_length_of_stack = _expected_length_of_stack;
  compute_via_step_size = false;
}

void Stack::set_expected_step_size(int _expected_step_size) {
  expected_step_size = _expected_step_size;
  compute_via_step_size = true;
}

void Stack::flip_start_at() { start_at_lower = !start_at_lower; }

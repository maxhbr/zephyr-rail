#include "Stack.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stack);

Stack::Stack(int step_size, const int start, const int end)
{
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
    if(length_of_stack > 1999) {
      return;
    }
    stepps_of_stack[length_of_stack] = stepps_of_stack[length_of_stack - 1] + step_size;
  }
  stepps_of_stack[length_of_stack] = end;
  length_of_stack++;
}

std::optional<int> Stack::get_current_step(){
  if(index_in_stack < 0 || length_of_stack <= index_in_stack) {
    return {};
  }
  return stepps_of_stack[index_in_stack];
}

void Stack::increment_step() {
  index_in_stack++;
}

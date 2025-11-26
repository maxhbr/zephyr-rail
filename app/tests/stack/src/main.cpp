#include <zephyr/ztest.h>

#include "Stack.h"

static void configure_bounds(Stack &stack, int lower, int upper) {
  stack.set_upper_bound(upper);
  stack.set_lower_bound(lower);
}

ZTEST(stack_suite, test_start_stack_uses_expected_length) {
  Stack stack;
  configure_bounds(stack, 0, 4000);
  stack.set_expected_length_of_stack(5);

  auto first_target = stack.start_stack();
  zassert_true(first_target.has_value(), "Stack failed to start");
  zassert_equal(first_target.value(), 0, "Unexpected first target");

  auto reported_length = stack.get_length_of_stack();
  zassert_true(reported_length.has_value());
  zassert_equal(reported_length.value(), 5, "Length mismatch");

  for (int step = 0; step < 5; ++step) {
    auto current = stack.get_current_target();
    zassert_true(current.has_value(), "Missing target at step %d", step);
    zassert_equal(current.value(), step * 1000, "Unexpected target at %d",
                  step);
    stack.increment_target();
  }

  zassert_false(stack.get_current_target().has_value(),
                "Targets should be exhausted");
  zassert_false(stack.stack_in_progress(), "Stack should be complete");
}

ZTEST(stack_suite, test_step_size_mode_respects_bounds) {
  Stack stack;
  configure_bounds(stack, 0, 5000);
  stack.set_expected_step_size(1000);

  auto start = stack.start_stack();
  zassert_true(start.has_value());

  auto length = stack.get_length_of_stack();
  zassert_true(length.has_value());
  zassert_true(length.value() >= 2, "Stack must contain bounds");

  int last_value = start.value();
  for (int i = 0; stack.stack_in_progress(); ++i) {
    auto current = stack.get_current_target();
    zassert_true(current.has_value(), "Missing value while stepping");
    zassert_true(current.value() >= last_value, "Stack target decreased at %d",
                 i);
    last_value = current.value();
    stack.increment_target();
  }

  zassert_equal(last_value, 5000, "Final target must match upper bound");
}

ZTEST(stack_suite, test_flip_start_changes_direction) {
  Stack stack;
  configure_bounds(stack, 1000, 6000);
  stack.set_expected_length_of_stack(3);

  auto first = stack.start_stack();
  zassert_true(first.has_value());
  zassert_equal(first.value(), 1000);

  stack.increment_target();
  stack.increment_target();
  stack.increment_target();
  zassert_false(stack.stack_in_progress());

  stack.flip_start_at();
  auto reversed = stack.start_stack();
  zassert_true(reversed.has_value());
  zassert_equal(reversed.value(), 6000,
                "Flip should start at previous upper bound");
}

ZTEST(stack_suite, test_invalid_configuration_blocks_start) {
  Stack stack;
  configure_bounds(stack, 0, 1000);
  stack.set_expected_length_of_stack(1);
  zassert_false(stack.start_stack().has_value(),
                "Stack should reject invalid length");
  zassert_false(stack.stack_in_progress());
}

ZTEST_SUITE(stack_suite, NULL, NULL, NULL, NULL, NULL);

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "StateMachine.h"

struct state_machine_fixture {
  StepperWithTarget *stepper;
  SonyRemote *remote;
  StateMachine *machine;
  struct k_thread thread_data;
  bool stop_thread;
  bool thread_exited;
};

K_THREAD_STACK_DEFINE(state_machine_thread_stack, 4096);

static void state_machine_thread(void *p1, void *, void *) {
  auto *fixture = static_cast<state_machine_fixture *>(p1);
  while (!fixture->stop_thread) {
    fixture->machine->run_state_machine();
    k_yield();
  }
  fixture->thread_exited = true;
}

static void *state_machine_setup(void) {
  auto *fixture = new state_machine_fixture;
  fixture->stepper = new StepperWithTarget(nullptr, 1, 1);
  fixture->remote = new SonyRemote();
  fixture->machine = new StateMachine(fixture->stepper, fixture->remote);
  fixture->stop_thread = false;
  fixture->thread_exited = false;

  k_thread_create(&fixture->thread_data, state_machine_thread_stack,
                  K_THREAD_STACK_SIZEOF(state_machine_thread_stack),
                  state_machine_thread, fixture, nullptr, nullptr,
                  K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
  return fixture;
}

static void state_machine_teardown(void *data) {
  auto *fixture = static_cast<state_machine_fixture *>(data);
  fixture->stop_thread = true;
  /* Wake the thread if it is waiting on events */
  event_pub(EVENT_STATUS);
  wait_for_condition([&]() { return fixture->thread_exited; }, 500);

  delete fixture->machine;
  delete fixture->remote;
  delete fixture->stepper;
  delete fixture;
}

template <typename Predicate>
static bool wait_for_condition(Predicate predicate, int timeout_ms = 250,
                               int step_ms = 5) {
  int waited = 0;
  while (waited <= timeout_ms) {
    if (predicate()) {
      return true;
    }
    k_msleep(step_ms);
    waited += step_ms;
  }
  return false;
}

static void reset_bounds(state_machine_fixture *fixture, int lower, int upper) {
  event_pub(EVENT_SET_LOWER_BOUND_TO, lower);
  event_pub(EVENT_SET_UPPER_BOUND_TO, upper);
  wait_for_condition([&]() {
    auto status = fixture->machine->get_stack_status();
    return status.lower_bound == lower && status.upper_bound == upper;
  });
}

ZTEST(state_machine_suite, test_go_pct_moves_stepper_within_bounds) {
  auto *fixture = static_cast<state_machine_fixture *>(ztest_get_user_data());
  reset_bounds(fixture, 0, 10000);

  event_pub(EVENT_GO_PCT, 50);
  zassert_true(wait_for_condition([&]() {
                 return fixture->stepper->get_target_position_nm() == 5000;
               }),
               "Stepper did not move to midpoint");
}

ZTEST(state_machine_suite, test_start_stack_with_requested_length) {
  auto *fixture = static_cast<state_machine_fixture *>(ztest_get_user_data());
  reset_bounds(fixture, 0, 4000);

  event_pub(EVENT_SET_WAIT_BEFORE_MS, 0);
  event_pub(EVENT_SET_WAIT_AFTER_MS, 0);
  event_pub(EVENT_START_STACK_WITH_LENGTH, 4);

  zassert_true(wait_for_condition(
                   [&]() {
                     auto status = fixture->machine->get_stack_status();
                     return status.index_in_stack.has_value() &&
                            status.length_of_stack == 4;
                   },
                   500),
               "Stack did not start");

  zassert_true(wait_for_condition(
                   [&]() {
                     return !fixture->machine->get_stack_status()
                                 .index_in_stack.has_value();
                   },
                   2000),
               "Stack did not complete");
}

ZTEST(state_machine_suite, test_go_relative_updates_target) {
  auto *fixture = static_cast<state_machine_fixture *>(ztest_get_user_data());
  reset_bounds(fixture, 0, 1000);
  fixture->stepper->set_target_position_nm(100);

  event_pub(EVENT_GO, 250);
  zassert_true(wait_for_condition([&]() {
                 return fixture->stepper->get_target_position_nm() == 350;
               }),
               "Relative move did not apply");
}

ZTEST_SUITE(state_machine_suite, NULL, state_machine_setup, NULL, NULL,
            state_machine_teardown);

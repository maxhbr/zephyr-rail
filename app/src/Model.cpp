#include "Model.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(model);

Model::Model(StepperWithTarget *_stepper) : stepper{_stepper}
{
  /* stepps_of_stack = (int *)malloc(sizeof(int) * 1000); */
}

void Model::log_state() {
  stepper->log_state();
  LOG_INF("bounds=(%i, %i)", lower_bound, upper_bound);
};

void Model::go(int dist) {
  LOG_MODULE_DECLARE(model);
  int target_position = stepper->go_relative(dist);
  LOG_INF("target: %i", target_position);
}

void Model::set_target_position(int _target_position) {
  stepper->set_target_position(_target_position);
}
int Model::get_target_position() { return stepper->get_target_position(); }

void Model::set_upper_bound(int _upper_bound) { upper_bound = _upper_bound; }
int Model::get_upper_bound() { return upper_bound; }
void Model::set_lower_bound(int _lower_bound) { lower_bound = _lower_bound; }
int Model::get_lower_bound() { return lower_bound; }

int Model::get_cur_position() { return stepper->get_position(); }

bool Model::is_in_target_position() {
  return get_cur_position() == get_target_position();
}

int Model::mk_stack(int step_size){
  Stack _stack(step_size, lower_bound, upper_bound);
  stack = &_stack;
}

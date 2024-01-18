#ifndef __STACK_H_
#define __STACK_H_

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

class Stack
{
  int length_of_stack = 0;
  int index_in_stack = 0;
  int *stepps_of_stack = (int *)malloc(sizeof(int) * 2000);
public:
  Stack(const int step_size, const int start, const int end);
  std::optional<int> get_current_step();
  void increment_step();
};

#endif // __STACK_H_


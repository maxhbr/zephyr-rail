#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_REBOOT
#include <zephyr/sys/reboot.h>
#endif
#include <zephyr/shell/shell.h>

#include "StateMachine.h"
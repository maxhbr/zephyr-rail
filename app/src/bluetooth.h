#pragma once

#include <zephyr/kernel.h>

#ifdef CONFIG_BT
class SonyRemote;
SonyRemote *init_bluetooth(void);
#endif
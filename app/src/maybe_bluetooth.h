#pragma once

#ifdef CONFIG_BT
#include "sony_remote/sony_remote.h"
#else
#include "sony_remote/fake_sony_remote.h"
#endif

SonyRemote *init_maybe_bluetooth(void);
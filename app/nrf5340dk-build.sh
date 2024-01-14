#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p builds
west build \
    -d builds/nrf5340dk \
    -b nrf5340dk_nrf5340_cpuapp \
    . \
    -t flash\
    -- -DSHIELD=buydisplay_3_5_tft_touch_arduino

#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p builds
west build \
    -d builds/nrf5340dk \
    -b nrf5340dk_nrf5340_cpuapp \
    . \
    -t flash\
    -- -DSHIELD=adafruit_2_8_tft_touch_v2


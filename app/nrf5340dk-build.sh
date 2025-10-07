#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p builds
set -x
west build \
    -d builds/nrf5340dk \
    -b nrf5340dk/nrf5340/cpuapp \
    . \
    -t flash\
    -- -DSHIELD=adafruit_2_8_tft_touch_v2


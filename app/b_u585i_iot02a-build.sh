#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p builds
set -x
west build \
    -d builds/b_u585i_iot02a \
    -b b_u585i_iot02a \
    . \
    -t flash\
    -- -DSHIELD=adafruit_2_8_tft_touch_v2

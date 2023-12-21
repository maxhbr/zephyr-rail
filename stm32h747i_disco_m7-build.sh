#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
west build \
    -d build-stm32h747i_disco_m7 \
    -b stm32h747i_disco_m7 \
    . \
    -t flash\
    -- -DSHIELD=st_b_lcd40_dsi1_mb1166
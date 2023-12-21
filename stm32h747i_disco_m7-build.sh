#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p builds
west build \
    -d builds/stm32h747i_disco_m7 \
    -b stm32h747i_disco_m7 \
    . \
    -t flash\
    -- -DSHIELD=st_b_lcd40_dsi1_mb1166
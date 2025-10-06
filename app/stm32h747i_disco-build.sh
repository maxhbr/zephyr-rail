#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p builds
set -x
west build \
    -d builds/stm32h747i_disco \
    -b stm32h747i_disco/stm32h747xx/m7 \
    --shield st_b_lcd40_dsi1_mb1166 \
    . -t flash

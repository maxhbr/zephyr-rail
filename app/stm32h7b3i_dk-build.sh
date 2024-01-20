#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p builds
west build \
    -d builds/stm32h7b3i_dk \
    -b stm32h7b3i_dk \
    . 

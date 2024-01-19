#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p builds
set -x
west build \
    -d builds/esp_wrover_kit \
    -b esp_wrover_kit \
    .
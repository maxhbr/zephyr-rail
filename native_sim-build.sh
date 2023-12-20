#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
west build \
    -d build-native_sim \
    -b native_sim \
    .
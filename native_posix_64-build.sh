#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
west build \
    -d build-native_posix_64 \
    -b native_posix_64 \
    .
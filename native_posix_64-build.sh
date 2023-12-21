#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p builds
west build \
    -d builds/native_posix_64 \
    -b native_posix_64 \
    .
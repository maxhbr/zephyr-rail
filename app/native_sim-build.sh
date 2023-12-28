#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p builds
west build \
    -d builds/native_sim \
    -b native_sim \
    .
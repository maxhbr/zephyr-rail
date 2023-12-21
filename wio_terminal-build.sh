#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p builds
west build \
    -d builds/wio_terminal \
    -b wio_terminal \
    .

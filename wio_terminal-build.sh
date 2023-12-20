#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
west build \
    -d build-wio_terminal \
    -b wio_terminal \
    .

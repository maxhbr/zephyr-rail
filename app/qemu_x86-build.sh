#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p builds
set -x
west build \
    -d builds/qemu_x86  \
    -b qemu_x86  \
    . \
    -t run

#!/usr/bin/env bash

set -euo pipefail
cd "$(dirname "$0")"
# or native_sim/native/64
build_dir="native_sim.build"
west build -b native_sim . --build-dir "$build_dir"
"$build_dir/zephyr/zephyr.exe" --bt-dev=127.0.0.1:9000
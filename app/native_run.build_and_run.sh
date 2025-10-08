#!/usr/bin/env bash

set -euo pipefail
cd "$(dirname "$0")"
# or native_sim/native/64
west build -b native_sim . --build-dir native_sim.build
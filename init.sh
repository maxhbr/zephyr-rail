#!/usr/bin/env bash
set -euo pipefail

ROOT="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

cd "$ROOT"
[[ -d ".west" ]] || west init -l ./app
west update --narrow #-f always
west blobs fetch hal_espressif
west config -l
west zephyr-export

export ZEPHYR_BASE="${ROOT}/zephyr"
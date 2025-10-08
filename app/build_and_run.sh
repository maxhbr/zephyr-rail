#!/usr/bin/env bash

set -euo pipefail
cd "$(dirname "$0")"
west build -b stm32h7b3i_dk --shield x_nucleo_idb05a1 .
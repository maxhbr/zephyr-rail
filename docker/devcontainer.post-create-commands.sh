#!/usr/bin/env bash

set -euo pipefail

west init || true
west update
west zephyr-export
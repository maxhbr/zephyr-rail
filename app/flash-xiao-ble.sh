#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

flash() {
    local label=XIAO-SENSE
    local dev=/dev/disk/by-label/"$label"
    local firmware="./builds/xiao_ble/zephyr/zephyr.uf2"

    echo "waiting for $dev"
    while sleep 1; do
        if [[ -e "$dev" ]]; then
            set -x
            sleep 1
            udisksctl mount -b "$dev"
            mnt="$(findmnt -nr -o target -S "$dev")"
            cp "$firmware" "$mnt"
            break
        fi
    done
}

west-xiao_ble-build
flash
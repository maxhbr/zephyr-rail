#!/usr/bin/env bash
set -euo pipefail

build() {
  local board="$1"
  shift
  local builddir="$1"
  shift

  west=west
  if command -v west-arm &> /dev/null; then
    west="west-arm"
  fi

  (
    set -x
    $west build \
        -d "$builddir" \
        -b "$board" \
        . "$@"
  )
}

flash_via_mount() {
    local label="$1"
    local dev=/dev/disk/by-label/"$label"
    local uf2="$2"

    if [[ ! -f "$uf2" ]]; then
      echo "file $uf2 does not exist"
      exit 1
    fi

    echo "waiting for $dev"
    while sleep 1; do
        if [[ -e "$dev" ]]; then
            sleep 1
            (
                set -x
                udisksctl mount -b "$dev"
                local mnt
                mnt="$(findmnt -nr -o target -S "$dev")"
                cp "$uf2" "$mnt"
            )
            break
        fi
    done
}


main() {
  local board="wio_terminal"
  local builddir="builds/$board"
  mkdir -p "$builddir"


  if [[ "$#" -gt 0 && "$1" == "flash" ]]; then
    # local uf2="$builddir/zephyr/zephyr.uf2"
    # flash_via_mount "Arduino" "$uf2"
    build "$board" "$builddir" -t flash
  else
    build "$board" "$builddir"
  fi
}

cd "$(dirname "$0")"
main "$@"

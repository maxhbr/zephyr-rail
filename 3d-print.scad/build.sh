#!/usr/bin/env bash

set -euo pipefail

buildPart() (
    local mode stl
    mode="$1"
    stl="$2"
    set -x
    openscad --hardwarnings \
        -o "$stl" \
        "rail.scad" \
        -D 'mode="'"$mode"'"'
)

render() {
    openscad --hardwarnings \
        --imgsize=3840,2160 \
        --projection=perspective \
        --colorscheme Tomorrow \
        "$@" \
        rail.scad \
        -D 'highRes=true'
}

version="${1:-"1.1.6-unstable"}"

build_all() {
    time buildPart "print" "rail_v${version}.stl" &
    time buildPart "ball_base_mount" "ball_base_mount_v${version}.stl" &
    time buildPart "ringclamp" "ringclamp_v${version}.stl" &

    wait
}

if [[ $# -gt 0 ]]; then
    case "$1" in
        all)
            time "$0" print &
            time "$0" ball_base_mount & 
            time "$0" ringclamp &
            wait
            ;;
        print|ball_base_mount|ringclamp)
            buildPart "$1" "${1}_v${version}.stl"
            ;;
        render)
            time render -o rail-1.png --camera=41.42,79.40,57.78,61.30,0,32.7,1579.6 &
            time render -o rail-2.png --camera=-22.35,75.49,-46.77,48.7,0,202.5,679.97 &
            wait
            ;;
        *)
            cat <<EOF
Build script for 3D printable parts of the Zephyr Rail

Usage:
    $0 all              Build all parts
    $0 print|ball_base_mount|ringclamp
        print           Build the main rail part
        ball_base_mount Build the ball base mount part
        ringclamp       Build the ring clamp part
    $0 render           Render images for the README
EOF
            exit 1
            ;;
    esac
    exit 0
fi

times
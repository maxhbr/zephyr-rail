#!/usr/bin/env nix-shell
#!nix-shell -i bash -p openscad
# shellcheck shell=bash

set -euo pipefail


version="1.1.7-unstable"
scad="rail.scad"

buildPart() (
    local mode stl
    mode="$1"
    stl="$2"
    set -x
    openscad --hardwarnings \
        -o "$stl" \
        "$scad" \
        -D 'mode="'"$mode"'"'
)

render() {
    openscad --hardwarnings \
        --imgsize=3840,2160 \
        --projection=perspective \
        --colorscheme Tomorrow \
        "$@" \
        "$scad" \
        -D 'highRes=true'
}

testScad() {
    echo "Testing $scad for syntax and integrity..."
    
    # Test syntax and basic compilation for each mode
    local modes=("print" "ball_base_mount" "ringclamp")
    local failed=0
    
    for mode in "${modes[@]}"; do
        echo "Testing mode: $mode"
        if ! openscad --hardwarnings \
            --export-format=stl \
            -o /dev/null \
            "$scad" \
            -D 'mode="'"$mode"'"' 2>&1; then
            echo "ERROR: Test failed for mode '$mode'"
            failed=1
        else
            echo "OK: Mode '$mode' passed"
        fi
    done
    
    if [[ $failed -eq 0 ]]; then
        echo "All tests passed successfully!"
        return 0
    else
        echo "Some tests failed!"
        return 1
    fi
}

build_all() {
    time buildPart "print" "rail_v${version}.stl" &
    time buildPart "ball_base_mount" "ball_base_mount_v${version}.stl" &
    time buildPart "ringclamp" "ringclamp_v${version}.stl" &

    wait
}

if [[ ! -f "$scad" ]]; then
    echo "ERROR: $scad not found!"
    exit 1
fi

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
            time render -o rail-1.png -D 'mode="develop"' --camera=41.42,79.40,57.78,61.30,0,32.7,1579.6 &
            time render -o rail-2.png -D 'mode="assembly"' --camera=-58.68,86.05,58.29,55.7,0,303.1,495.7 &
            wait
            ;;
        test)
            testScad
            ;;
        *)
            cat <<EOF
Build script for 3D printable parts of the Zephyr Rail

Usage:
    \$0 all              Build all parts
    \$0 print|ball_base_mount|ringclamp
        print           Build the main rail part
        ball_base_mount Build the ball base mount part
        ringclamp       Build the ring clamp part
    \$0 render           Render images for the README
    \$0 test             Test $scad for syntax and integrity
EOF
            exit 1
            ;;
    esac
fi

times

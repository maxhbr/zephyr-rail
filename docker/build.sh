#!/usr/bin/env bash
set -euo pipefail

oci_cmd=docker

context="$( cd -- "$( dirname -- "$(readlink -f "${BASH_SOURCE[0]}")" )" &> /dev/null && pwd )"
path="app"
repo="$(readlink -f "${context}/../${path}")"
tag="maxhbr/zephyr-rail-zephyrbuilder"

oci_build() {
    $oci_cmd build "$context" --tag "$tag"
}

oci_west() (
    set -x
    $oci_cmd run --rm -it \
        --user user:user \
        -v "${repo}:/workspaces/${path}" \
        --workdir="/workspaces/${path}" \
        -e path="${path}" \
        "$tag" \
        west $@
)

if [[ $# -gt 0 && "$1" == "build" ]]; then
    shift
    oci_build
fi

[[ -d ".west" ]] || oci_west init
oci_west update #-f always
oci_west config -l
oci_west zephyr-export
oci_west build \
    -s "." \
    -p always \
    -d ./build

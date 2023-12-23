#!/usr/bin/env bash
set -euo pipefail

oci_cmd=docker

oci_build() {
    local tag="$1"; shift
    local context="$1"; shift
    $oci_cmd build "$context" --tag "$tag"
}

oci_run() {
    local tag="$1"; shift
    local repo="$1"; shift
    $oci_cmd run --rm -it \
        --user user:user \
        -v "${repo}:${repo}" \
        --workdir="${repo}" \
        -e ZEPHYR_BASE="${repo}/zephyr"\
        "$tag" \
        $@
}

oci_west_init() {
    local tag="$1"; shift
    local repo="$1"; shift

    oci_run "$tag" "$repo" ./init.sh
}

oci_west() (
    local tag="$1"; shift
    local repo="$1"; shift

    set -x
    oci_run "$tag" "$repo" west $@
)

main() {
    local repo="$( cd -- "$( dirname -- "$(readlink -f "${BASH_SOURCE[0]}")" )" &> /dev/null && pwd )"
    local project_name="$(basename "$repo")"
    local context="${repo}/docker"

    # local path="$(cat "${repo}/west.yml" | yq -r '.manifest.self.path')"
    path=app
    local tag="maxhbr/${project_name}-zephyrbuilder"

    if [[ $# -gt 0 && "$1" == "--build" ]]; then
        shift
        oci_build "$tag" "$context"
    fi

    oci_west_init "$tag" "$repo"

    oci_west "$tag" "$repo" \
        build \
        -s "$path" \
        -p always \
        -d ./build
}

main $@

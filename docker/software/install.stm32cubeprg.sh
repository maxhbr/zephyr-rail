#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

# see https://www.st.com/en/development-tools/stm32cubeprog.html for terms of use and download location

vMajor=2
vMinor=14
vPatch=0

vZip="v${vMajor}-${vMinor}-${vPatch}"
vExec="${vMajor}.${vMinor}.${vPatch}"
zip="en.stm32cubeprg-lin-${vZip}.zip"
if [[ ! -f "$zip" ]]; then
    echo "no ${zip} => skip install"
    exit 0
fi

tmpdir="$(mktemp -d)"
unzip "$zip" -d "$tmpdir"
exec "$tmpdir/SetupSTM32CubeProgrammer-${vExec}.linux"

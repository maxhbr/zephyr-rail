#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

SEGGER_JLINK_VERSION="V788l"
wget --post-data 'accept_license_agreement=accepted&submit=Download+software' \
    -nv "https://www.segger.com/downloads/jlink/JLink_Linux_${SEGGER_JLINK_VERSION}_x86_64.deb" \
    -O "/tmp/JLink_Linux_x86_64.deb"
( dpkg -i "/tmp/JLink_Linux_x86_64.deb" || apt-get -f install -y || true )
rm "/tmp/JLink_Linux_x86_64.deb"
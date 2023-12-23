#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

NRF_COMMAND_LINE_TOOLS_URL="https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/desktop-software/nrf-command-line-tools/sw/versions-10-x-x/10-22-1/nrf-command-line-tools_10.22.1_amd64.deb"
wget \
    -nv "$NRF_COMMAND_LINE_TOOLS_URL"\
    -O "/tmp/nrf-command-line-tools.deb"
( dpkg -i "/tmp/nrf-command-line-tools.deb" || apt-get -f install -y || true )
rm "/tmp/nrf-command-line-tools.deb"
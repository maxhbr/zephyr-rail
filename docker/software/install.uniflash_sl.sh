#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

# apt-get install -y libc6-i386 libusb-0.1-4 libgconf-2-4 libncurses5 libpython2.7 libtinfo5

UNIFLASH_SL_URL="https://dr-download.ti.com/software-development/software-programming-tool/MD-QeJBJLj8gq/8.3.0/uniflash_sl.8.3.0.4307.run"
wget \
     -nv "$UNIFLASH_SL_URL" \
     -O "/opt/uniflash_sl.run"
chmod +x "/opt/uniflash_sl.run"
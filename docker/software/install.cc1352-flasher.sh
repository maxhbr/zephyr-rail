#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

if [ "$EUID" -ne 0 ]
    then echo "Please run as root"
    exit
fi

pip3 install pyserial intelhex python-magic
git clone --depth 1 https://git.beagleboard.org/beagleconnect/cc1352-flasher /opt/cc1352-flasher
ln -s /opt/cc1352-flasher/cc1352-flasher.py /usr/bin/cc1352-flasher.py
ln -s /opt/cc1352-flasher/cc1352-flasher.py /usr/bin/cc1352-flasher
ARG ZEPHYR_CI_IMAGE_VERSION="v0.26.8"

FROM ghcr.io/zephyrproject-rtos/zephyr-build:${ZEPHYR_CI_IMAGE_VERSION}

USER root

ARG USERNAME=user
ARG USER_UID=1000
ARG USER_GID=$USER_UID
COPY library-scripts/*.sh library-scripts/*.env /tmp/library-scripts/

RUN set -x \
    && apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && /bin/bash /tmp/library-scripts/common-debian.sh "${INSTALL_ZSH}" "${USERNAME}" "${USER_UID}" "${USER_GID}" "${UPGRADE_PACKAGES}" "true" "true"\
    && usermod --shell "$(which zsh)" "${USERNAME}" \
    && usermod -a -G dialout "${USERNAME}" \

    && apt-get install -y \
            minicom picocom \
            wget \
            unzip \
            udev \
            libusb-1.0-0 \
            openocd \
    
    && apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/* /tmp/library-scripts

COPY software/ /opt/zephyr-template-software
RUN set -x \
 && apt-get update && export DEBIAN_FRONTEND=noninteractive \
 && apt-get install -y udev \
 && /lib/systemd/systemd-udevd --daemon manually \

 && cd /opt/zephyr-template-software/ \
 && ./install.cc1352-flasher.sh \
 && ./install.rf-command-line-tools.sh \
 && ./install.jlink-linux.sh \
 && ./install.uniflash_sl.sh \
 && apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/*

USER "${USERNAME}"

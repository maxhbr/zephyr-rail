FROM ghcr.io/zephyrproject-rtos/ci:v0.24.6

ARG USERNAME=user
ARG USER_UID=1000
ARG USER_GID=$USER_UID
COPY library-scripts/*.sh library-scripts/*.env /tmp/library-scripts/
RUN set -x \
    && apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && /bin/bash /tmp/library-scripts/common-debian.sh "${INSTALL_ZSH}" "${USERNAME}" "${USER_UID}" "${USER_GID}" "${UPGRADE_PACKAGES}" "true" "true"\
    && usermod --shell "$(which zsh)" "${USERNAME}" \

    && apt-get install -y minicom picocom \

# 	&& wget \
#             -nv https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/desktop-software/nrf-command-line-tools/sw/versions-10-x-x/10-18-1/nrf-command-line-tools_10.18.1_amd64.deb \
#             -O "/tmp/nrf-command-line-tools.deb" \
# 	&& ( dpkg -i "/tmp/nrf-command-line-tools.deb" || apt-get -f install -y || true) \
# 	&& rm "/tmp/nrf-command-line-tools.deb"

# 	&& wget --post-data 'accept_license_agreement=accepted&submit=Download+software' \
# 			-nv "https://www.segger.com/downloads/jlink/JLink_Linux_V784a_x86_64.deb" \
# 			-O "/tmp/JLink_Linux_x86_64.deb" \
# 	&& ( dpkg -i "/tmp/JLink_Linux_x86_64.deb" || apt-get -f install -y || true ) \
# 	&& rm "/tmp/JLink_Linux_x86_64.deb"

    && apt-get autoremove -y && apt-get clean -y && rm -rf /var/lib/apt/lists/* /tmp/library-scripts
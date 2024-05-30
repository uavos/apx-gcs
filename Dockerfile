FROM ubuntu:22.04
LABEL description="Linux development environment for APX Ground Control"
LABEL maintainer="sa@uavos.com"

ENV TERM=xterm-color
ENV DEBIAN_FRONTEND=noninteractive
ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

# basic APT packages
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates bc \
    build-essential rsync curl git make ninja-build pkg-config python3-pip \
    fuse \
    bc \
    && \
    rm -Rf /var/cache/apt

# architecture detect
RUN arch="$(dpkg --print-architecture)" && \
    case "${arch##*-}" in \
    amd64) arch='x86_64' ;; \
    arm64) arch='aarch64' arch_qt='_arm64' ;; \
    armhf) arch='armv7l' ;; \
    i386) arch='i686' ;; \
    *) echo >&2 "error: unsupported architecture: ${arch}"; exit 1 ;; \
    esac && \
    echo "Detected architecture: ${arch}" && \
    echo "${arch}" > /arch && \
    echo "${arch_qt}" > /arch_qt

# CMAKE
ARG CMAKE_VERSION=3.29.3
RUN curl -L https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-$(cat /arch).sh --output /tmp/install-cmake.sh && \
    sh /tmp/install-cmake.sh --skip-license --prefix=/usr/local &&\
    cmake --version

# LINUXDEPLOY
RUN curl -L https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$(cat /arch).AppImage --output /usr/local/bin/linuxdeploy && \
    chmod +x /usr/local/bin/linuxdeploy && \
    curl -L https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$(cat /arch).AppImage --output /usr/local/bin/linuxdeploy-plugin-qt && \
    chmod +x /usr/local/bin/linuxdeploy-plugin-qt && \
    curl -L https://github.com/NixOS/patchelf/releases/download/0.14.5/patchelf-0.14.5-$(cat /arch).tar.gz --output /tmp/patchelf.tar.gz && \
    cd /tmp && tar -xf patchelf.tar.gz && \
    mv bin/patchelf /usr/local/bin/ && rm -rf *


# APPIMAGETOOL
RUN curl -L https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-$(cat /arch).AppImage --output /usr/local/bin/appimagetool && \
    chmod +x /usr/local/bin/appimagetool


# libs: apt
RUN apt-get install -y --no-install-recommends \
    libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-shape0 libxcb-xinerama0 \
    libodbc1 libpq5 \
    libcups2 \
    libz-dev libsdl2-dev zsync \
    \
    libgstreamer1.0-0 \
    gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav gstreamer1.0-tools \
    \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-x gstreamer1.0-gl gstreamer1.0-libav gstreamer1.0-pulseaudio libgstreamer-plugins-base1.0-dev \
    libegl1-mesa-dev libgles2-mesa-dev libxkbcommon-x11-dev libspeechd-dev libffi-dev \
    libicu-dev libxcb-cursor-dev libmariadb3 \
    file \
    && rm -Rf /var/cache/apt

# libs: to include in release package
ENV LIBS_DIST_DIR=/dist
RUN mkdir -p $LIBS_DIST_DIR
RUN cd $LIBS_DIST_DIR && apt-get download libsdl2-2.0-0 libsndio7.0


# python tools
RUN pip3 install networkx simplejson jinja2 pyyaml

# Qt packages
RUN apt install -y --no-install-recommends \
    python3-dev && rm -Rf /var/cache/apt

ARG VERSION_QT=6.7.1
RUN pip install aqtinstall &&\
    aqt install-qt linux$(cat /arch_qt) desktop ${VERSION_QT} -m \
    qtshadertools qt5compat qtcharts qtmultimedia \
    qtspeech qtlocation qtpositioning qtserialport &&\
    rsync -av /${VERSION_QT}/*/ /usr/local/ && rm -Rf /${VERSION_QT}

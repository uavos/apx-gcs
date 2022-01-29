FROM ubuntu:latest
LABEL description="Linux development environment for APX Ground Control"
LABEL maintainer="sa@uavos.com"

ENV TERM=xterm-color
ENV DEBIAN_FRONTEND=noninteractive

# basic APT packages
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    build-essential rsync curl git make ninja-build pkg-config python3-pip \
    && \
    rm -Rf /var/cache/apt

# cmake
ARG CMAKE_VERSION=3.22.1

RUN curl -L https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.sh --output /tmp/install-cmake.sh && \
    sh /tmp/install-cmake.sh --skip-license --prefix=/usr/local &&\
    cmake --version

# Qt packages
ARG VERSION_QT=5.15.2

ARG INSTALL_QT_SRC=https://raw.githubusercontent.com/qbs/qbs/master/scripts/install-qt.sh
RUN apt-get install -y --no-install-recommends p7zip-full && rm -Rf /var/cache/apt
RUN curl -L ${INSTALL_QT_SRC} --output /tmp/install-qt.sh && \
    chmod +x /tmp/install-qt.sh && \
    /tmp/install-qt.sh --version ${VERSION_QT} --directory /tmp/qt \
        qtbase qtdeclarative qttools \
        quickcontrols2 serialport multimedia speech svg location \
        graphicaleffects qtcharts icu \
        && \
    rsync -av /tmp/qt/${VERSION_QT}/*/ /usr/local/ && rm -Rf /tmp/*


# Qt deploy tools
RUN apt-get install -qq -y --no-install-recommends fuse && rm -Rf /var/cache/apt

ARG LINUXDEPLOYQT_SRC=https://github.com/probonopd/linuxdeployqt/releases/download/6/linuxdeployqt-6-x86_64.AppImage
RUN curl -L ${LINUXDEPLOYQT_SRC} --output /usr/local/bin/linuxdeployqt && \
    chmod +x /usr/local/bin/linuxdeployqt

ARG APPIMAGETOOL_SRC=https://github.com/AppImage/AppImageKit/releases/download/12/appimagetool-x86_64.AppImage
RUN curl -L ${APPIMAGETOOL_SRC} --output /usr/local/bin/appimagetool && \
    chmod +x /usr/local/bin/appimagetool

# ARG LINUXDEPLOY_SRC=https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
# RUN curl -L ${LINUXDEPLOY_SRC} --output /usr/local/bin/linuxdeploy && \
# 	chmod +x /usr/local/bin/linuxdeploy

# ARG LINUXDEPLOY_PLUGIN_QT_SRC=https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
# RUN curl -L ${LINUXDEPLOY_PLUGIN_QT_SRC} --output /usr/local/bin/linuxdeploy-plugin-qt && \
# 	chmod +x /usr/local/bin/linuxdeploy-plugin-qt

# ARG LINUXDEPLOY_PLUGIN_GSTREAMER_SRC=https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gstreamer/master/linuxdeploy-plugin-gstreamer.sh
# RUN curl -L ${LINUXDEPLOY_PLUGIN_GSTREAMER_SRC} --output /usr/local/bin/linuxdeploy-plugin-gstreamer.sh && \
# 	chmod +x /usr/local/bin/linuxdeploy-plugin-gstreamer.sh


# libs: appimageupdate
RUN apt-get install -y --no-install-recommends \
    xxd wget automake desktop-file-utils librsvg2-dev libfuse-dev patchelf \
    libcurl4-openssl-dev libssl-dev libtool libglib2.0-dev libcairo-dev \
    argagg-dev libgcrypt-dev \
    && rm -Rf /var/cache/apt

RUN git clone --depth=1 --recurse-submodules https://github.com/AppImage/AppImageUpdate.git && \
    cd AppImageUpdate && \
        sed -i 's/\"HTTP\/1\", 6/\"HTTP\/1\", 4/' lib/zsync2/src/legacy_http.c && \
        cmake -H. -Bbuild && \
        cmake --build build --target install && \
	find build -name '*.a' -exec cp "{}" /usr/local/lib/ \; && \
	cd .. && rm -Rf AppImageUpdate


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
    gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools \
    \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-x gstreamer1.0-gl gstreamer1.0-libav gstreamer1.0-pulseaudio libgstreamer-plugins-base1.0-dev \
    libegl1-mesa-dev libgles2-mesa-dev libxkbcommon-x11-dev libspeechd-dev libffi-dev \
    && rm -Rf /var/cache/apt

# libs: to include in release package
ENV LIBS_DIST_DIR=/dist
RUN mkdir -p $LIBS_DIST_DIR
RUN cd $LIBS_DIST_DIR && apt-get download libsdl2-2.0-0 libsndio7.0


# python
RUN pip3 install networkx simplejson jinja2 pyyaml


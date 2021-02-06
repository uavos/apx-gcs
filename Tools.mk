FRAMEWORKS_DIR := $(if $(FRAMEWORKS_DIR),$(FRAMEWORKS_DIR),/Library/Frameworks)

VERSION_QT = 5.15.2

VERSION_GST = 1.18.3
VERSION_SPARKLE = 1.22.0
VERSION_SDL2 = 2.0.14

LINUXDEPLOYQT_SRC = https://github.com/probonopd/linuxdeployqt/releases/download/7/linuxdeployqt-7-x86_64.AppImage


#detect OS: osx, linux
HOST_OS_LIST = linux osx
ifndef HOST_OS
UNAME_S = $(shell uname -s)
ifeq ($(UNAME_S),Linux)
HOST_OS = linux
endif
ifeq ($(UNAME_S),Darwin)
HOST_OS = osx
endif
endif
ifeq ($(filter $(HOST_OS),$(HOST_OS_LIST)),)
$(error Bad HOST_OS = $(HOST_OS) [$(HOST_OS_LIST)])
endif

CACHE_DIR := $(if $(CACHE_DIR),$(CACHE_DIR),$(BUILD_DIR)/tools-cache)

APT := sudo DEBIAN_FRONTEND=noninteractive apt-get
APT_INSTALL := $(APT) install -y --no-install-recommends

export LIBS_DIST_DIR := $(CURDIR)/build/dist


install-tools: install-tools-python install-tools-$(HOST_OS)


install-tools-python:
	@pip3 install networkx simplejson jinja2 pyyaml dmgbuild

install-tools-osx: install-tools-brew-osx

install-tools-linux: install-tools-apt-linux install-appimageupdate-linux install-dist-linux


#######################################################
# LINUX
#######################################################
install-tools-apt-linux:
	@$(APT_INSTALL) \
		ninja-build \
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
		libegl1-mesa-dev libgles2-mesa-dev libxkbcommon-x11-dev libspeechd-dev libffi-dev

install-appimageupdate-linux:
	@$(APT_INSTALL) libcurl4-gnutls-dev libssl-dev libtool libglib2.0-dev libcairo-dev
	@sudo rm -rf /usr/src/AppImageUpdate
	@sudo git clone --depth=1 --recurse-submodules https://github.com/AppImage/AppImageUpdate.git /usr/src/AppImageUpdate
	@cd /usr/src/AppImageUpdate && sudo cmake -H. -Bbuild && sudo cmake --build build --target install

# dist packages to add to bundle
install-dist-linux:
	@rm -rf $(LIBS_DIST_DIR) && mkdir -p $(LIBS_DIST_DIR)
	@cd $(LIBS_DIST_DIR) && $(APT) download libsdl2-2.0-0 libsndio6.1


#######################################################
# OSX
#######################################################
install-tools-brew-osx:
	@brew install ninja




#######################################################
# install Qt and app deploy tools
install-qt:
	@$(TOOLS_DIR)/install-qt.sh --version $(VERSION_QT) --directory $(QT_INSTALL_DIR) $(QT_PACKAGES) ${QT_PACKAGES_${HOST_OS}} | tail -n 1 > $(QT_INSTALL_DIR)/path_qt
	@make $@-$(HOST_OS)

install-qt-osx:

install-qt-linux: install-qt-linux-fix
	@mkdir -p $(CACHE_DIR)
	$(WGET) $(LINUXDEPLOYQT_SRC) --output $(CACHE_DIR)/linuxdeployqt && \
		chmod +x $(CACHE_DIR)/linuxdeployqt && \
		mv $(CACHE_DIR)/linuxdeployqt $(dir $(QMAKE_BIN))
	$(WGET) $(APPIMAGETOOL_SRC) --output $(CACHE_DIR)/appimagetool && \
		chmod +x $(CACHE_DIR)/appimagetool && \
		mv $(CACHE_DIR)/appimagetool $(dir $(QMAKE_BIN))
install-qt-linux-fix:
	@cd $(dir $(QMAKE_BIN))/../plugins/sqldrivers && \
		rm -f libqsqlpsql.so libqsqlmysql.so libqsqlodbc.so

#######################################################
# GStreamer framework
install-gst-osx: $(FRAMEWORKS_DIR)/GStreamer.framework/Versions/1.0/GStreamer

GST_PKGS = \
	gstreamer-1.0-$(VERSION_GST)-x86_64.pkg \
	gstreamer-1.0-devel-$(VERSION_GST)-x86_64.pkg

$(FRAMEWORKS_DIR)/GStreamer.framework/%: $(GST_PKGS:%=$(CACHE_DIR)/%)
	@echo "Installing GStreamer..."
	@for pkg in $^ ; do \
		sudo installer -package $$pkg -target / ; \
	done

$(CACHE_DIR)/gstreamer-%:
	@mkdir -p $(CACHE_DIR)
	$(WGET) https://gstreamer.freedesktop.org/data/pkg/osx/$(VERSION_GST)/$(notdir $@) --output $@


#######################################################
# Sparkle framework
install-sparkle-osx: $(FRAMEWORKS_DIR)/Sparkle.framework/Versions/A/Sparkle

SPARKLE_PKG = Sparkle-$(VERSION_SPARKLE).tar.bz2

$(FRAMEWORKS_DIR)/Sparkle.framework/%: $(SPARKLE_PKG:%=$(CACHE_DIR)/%)
	@echo "Installing Sparkle..."
	@mkdir -p $(BUILD_DIR)/sparkle
	@cd $(BUILD_DIR)/sparkle; \
		bunzip2 -k -c $< | tar xopf -;\
		sudo cp -a Sparkle.framework /Library/Frameworks/

$(CACHE_DIR)/Sparkle-%:
	@mkdir -p $(CACHE_DIR)
	$(WGET) https://github.com/sparkle-project/Sparkle/releases/download/$(VERSION_SPARKLE)/$(notdir $@) --output $@


#######################################################
# SDL framework
install-sdl-osx: $(FRAMEWORKS_DIR)/SDL2.framework/Versions/A/SDL2

SDL_PKG = SDL2-$(VERSION_SDL2).dmg

$(FRAMEWORKS_DIR)/SDL2.framework/%: $(SDL_PKG:%=$(CACHE_DIR)/%)
	@echo "Installing SDL2..."
	hdiutil attach -noverify $<
	sudo cp -a /Volumes/SDL2/SDL2.framework /Library/Frameworks/

$(CACHE_DIR)/SDL2-%:
	@mkdir -p $(CACHE_DIR)
	curl -L https://www.libsdl.org/release/$(notdir $@) --output $@



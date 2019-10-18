###########################################################################
#
# APX project makefile to build Ground Control application
#
###########################################################################
include ../Rules.mk

QT_VERSION := 5.13.0
GCS_BUILD_DIR := $(BUILD_DIR)/gcs


GCS_STAMP = $(GCS_BUILD_DIR)-$(GIT_VERSION)

# compile app bundle and SDK archive
build: $(GCS_STAMP)-build

$(GCS_STAMP)-build: qt-version qbs-version
	$(QBS) build -d $(GCS_BUILD_DIR) -f gcs.qbs config:release $(QBS_OPTS)
	@touch $@

install: build deploy #$(GCS_STAMP)-install

$(GCS_STAMP)-install: build deploy
	@mkdir -p $(BUILD_DIR_OUT)
	@install $(BUILD_DIR)/gcs/release/install-root/packages/* $(BUILD_DIR_OUT)
	@touch $@

# populate
deploy: libsqt-$(HOST_OS) image-$(HOST_OS)

# update bundle with Qt libs
libsqt-osx: build


# build DMG image
image-osx: build



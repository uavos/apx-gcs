###########################################################################
#
# APX project makefile to build Ground Control application release package
#
###########################################################################
include ../Rules.mk

#QT_VERSION := 5.13.0
GCS_BUILD_DIR := $(BUILD_DIR)/gcs
GCS_ROOT_DIR := $(GCS_BUILD_DIR)/release/install-root


GCS_STAMP = $(GCS_BUILD_DIR)-$(GIT_VERSION)-$(HOST_OS)

# compile app bundle and SDK archive
build: qt-version qbs-version $(GCS_STAMP)-build

$(GCS_STAMP)-build:
	@mkdir -p $(BUILD_DIR)
	$(QBS) build -d $(GCS_BUILD_DIR) -f gcs.qbs config:release $(QBS_OPTS)
	@touch $@

install: deploy #$(GCS_STAMP)-install

$(GCS_STAMP)-install: deploy
	@mkdir -p $(BUILD_DIR_OUT)
	@install $(GCS_ROOT_DIR)/packages/* $(BUILD_DIR_OUT)
	@touch $@

# populate
deploy: build libs-$(HOST_OS) image-$(HOST_OS)

# update bundle with Qt libs
libs-osx:
	@echo "Deploy app libs..."
	@python $(TOOLS_DIR)/deploy_app_libs.py --appdata=$(GCS_ROOT_DIR)/appdata.json


# build DMG image
image-osx: build



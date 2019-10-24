###########################################################################
#
# APX project makefile to build Ground Control application release package
#
###########################################################################
all: build

include ../Rules.mk

# Environment variables:
# [QTDIR] - qt installation directory, e.g. /opt/Qt/5.13.0/clang_64
# [QBSDIR] - qbs bin directory, e.g. /opt/Qt/Tools/QtCreator/bin
# env vars can be omitted if found in PATH

GCS_BUILD_DIR := $(BUILD_DIR)/gcs-$(HOST_OS)
GCS_ROOT_DIR := $(GCS_BUILD_DIR)/release/install-root

APP_DATA = $(GCS_ROOT_DIR)/appdata.json


build: qt-version qbs-version $(APP_DATA)

$(APP_DATA): qbs-init
	@mkdir -p $(GCS_BUILD_DIR)
	@$(QBS) build -d $(GCS_BUILD_DIR) -f gcs.qbs config:release $(QBS_OPTS)


clean: FORCE
	@rm -rf $(GCS_BUILD_DIR)

# prepare bundle with libs and frameworks
deploy: build deploy-app deploy-$(HOST_OS)
	@mkdir -p $(BUILD_DIR_OUT)
	@install $(GCS_ROOT_DIR)/packages/* $(BUILD_DIR_OUT)

deploy-clean: 
	@rm -rf $(GCS_ROOT_DIR)/*

deploy-app: $(APP_DATA)
	@python $(TOOLS_DIR)/deploy/deploy_app.py --appdata=$< $(LIBS_DIST_DIR:%=--dist=%) $(CODE_IDENTITY:%=--sign=%)


# build dist image
deploy-osx: $(APP_DATA)
	@python $(TOOLS_DIR)/deploy/deploy_dmg.py --appdata=$<

deploy-linux: $(APP_DATA)
	@python $(TOOLS_DIR)/deploy/deploy_appimage.py --appdata=$<



# toolchain
qbs-init: FORCE
	@echo "QBS Toolchain init..."
	@$(QBS) setup-toolchains --detect
	@$(QBS) setup-qt $(QMAKE) qt-release
	@$(QBS) config defaultProfile qt-release


# update materialdesignicons
# https://github.com/Templarian/MaterialDesign-Webfont
ICONS_PATH := $(CURDIR)/resources/icons/material-icons

update-icons:
	@echo "Updating icons..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR); \
		curl -L https://codeload.github.com/Templarian/MaterialDesign-Webfont/zip/master --output icons.zip; \
		rm -fR $(BUILD_DIR)/MaterialDesign-Webfont-master;\
		unzip icons.zip
	@cp -f $(BUILD_DIR)/MaterialDesign-Webfont-master/fonts/materialdesignicons-webfont.ttf $(ICONS_PATH).ttf
	@python ../tools/make_icons.py --src=$(BUILD_DIR)/MaterialDesign-Webfont-master/css/materialdesignicons.css --dest=$(ICONS_PATH).json



.PHONY: clean deploy-clean qbs-init update-icons

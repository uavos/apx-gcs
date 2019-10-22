###########################################################################
#
# APX project makefile to build Ground Control application release package
#
###########################################################################
include ../Rules.mk

#QT_VERSION := 5.13.0
GCS_BUILD_DIR := $(BUILD_DIR)/gcs-$(HOST_OS)
GCS_ROOT_DIR := $(GCS_BUILD_DIR)/release/install-root


GCS_STAMP = $(GCS_BUILD_DIR)-$(GIT_VERSION)-$(HOST_OS)
APP_DATA = $(GCS_ROOT_DIR)/appdata.json

# compile app bundle and SDK archive
build: qt-version qbs-version $(APP_DATA)

$(APP_DATA):
	@mkdir -p $(BUILD_DIR)
	$(QBS) build -d $(GCS_BUILD_DIR) -f gcs.qbs config:release $(QBS_OPTS)

install: deploy #$(GCS_STAMP)-install

$(GCS_STAMP)-install: deploy
	@mkdir -p $(BUILD_DIR_OUT)
	@install $(GCS_ROOT_DIR)/packages/* $(BUILD_DIR_OUT)
	@touch $@

# prepare bundle with libs and frameworks
deploy: build deploy-app deploy-$(HOST_OS)

deploy-clean: 
	@rm -rf $(GCS_ROOT_DIR)/*

deploy-app: $(APP_DATA)
	@python $(TOOLS_DIR)/deploy/deploy_app.py --appdata=$< --dist=$(APX_DIR)/../dist $(CODE_IDENTITY:%=--sign=%)


# build dist image
deploy-osx: $(APP_DATA)
	@python $(TOOLS_DIR)/deploy/deploy_dmg.py --appdata=$<

deploy-linux: $(APP_DATA)
	@python $(TOOLS_DIR)/deploy/deploy_appimage.py --appdata=$<



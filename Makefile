
BUILD_DIR := $(if $(BUILD_DIR),$(BUILD_DIR),build-release)
TOOLS_DIR := $(realpath $(CURDIR)/tools)

CMAKE := cmake

APX_PROJECT_TITLE := "APX Ground Control"

all: configure build

configure:
	@$(CMAKE) -DCMAKE_BUILD_TYPE=Release -H. -B$(BUILD_DIR) -G Ninja

build:
	@$(CMAKE) --build $(BUILD_DIR)

bundle:
	$(CMAKE) --build $(BUILD_DIR) --target deploy_bundle

package:
	$(CMAKE) --build $(BUILD_DIR) --target deploy_package

release-package: configure package



# update materialdesignicons
# https://github.com/Templarian/MaterialDesign-Webfont
ICONS_PATH := $(CURDIR)/resources/icons/material-icons
ICONS_BUILD_DIR := $(BUILD_DIR)/icons

update-icons:
	@echo "Updating icons..."
	@mkdir -p $(ICONS_BUILD_DIR)
	@cd $(ICONS_BUILD_DIR); \
		curl -L https://codeload.github.com/Templarian/MaterialDesign-Webfont/zip/master --output icons.zip; \
		rm -fR MaterialDesign-Webfont-master;\
		unzip icons.zip
	@cp -f $(ICONS_BUILD_DIR)/MaterialDesign-Webfont-master/fonts/materialdesignicons-webfont.ttf $(ICONS_PATH).ttf
	@python $(TOOLS_DIR)/make_icons.py --src=$(ICONS_BUILD_DIR)/MaterialDesign-Webfont-master/css/materialdesignicons.css --dest=$(ICONS_PATH).json



.PHONY: update-icons build package configure


include Docker.mk
include shared/tools/Release.mk
include shared/tools/Submodules.mk
include docs/Docs.mk

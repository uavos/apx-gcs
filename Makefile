TOP_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

BUILD_DIR := $(if $(BUILD_DIR),$(BUILD_DIR),$(TOP_DIR)/../build-gcs)



all:
	echo "QBS project"


# update materialdesignicons
# https://github.com/Templarian/MaterialDesign-Webfont
ICONS_PATH := $(CURDIR)/resources/icons/material-icons
ICONS_BUILD_DIR := $(BUILD_DIR)/icons

update-icons:
	@echo "Updating icons..."
	@mkdir -p $(ICONS_BUILD_DIR)
	@cd $(ICONS_BUILD_DIR); \
		curl -L https://codeload.github.com/Templarian/MaterialDesign-Webfont/zip/master --output icons.zip; \
		rm -fR $(ICONS_BUILD_DIR)/MaterialDesign-Webfont-master;\
		unzip icons.zip
	@cp -f $(ICONS_BUILD_DIR)/MaterialDesign-Webfont-master/fonts/materialdesignicons-webfont.ttf $(ICONS_PATH).ttf
	@python tools/make_icons.py --src=$(ICONS_BUILD_DIR)/MaterialDesign-Webfont-master/css/materialdesignicons.css --dest=$(ICONS_PATH).json



.PHONY: update-icons

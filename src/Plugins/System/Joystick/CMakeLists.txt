apx_plugin(QT Widgets)

apx_install_res(${APX_RESOURCES_DIR} templates/joystick.json)

# Joysticks are accessed through OS services only, no external libraries:
# - Linux: kernel evdev interface (/dev/input/event*)
# - macOS: IOKit HID Manager (system framework)

if(APPLE)
    target_link_libraries(${MODULE} PRIVATE "-framework IOKit" "-framework CoreFoundation")

elseif(UNIX AND NOT APPLE)
    # nothing to link

elseif(WIN32)
    message(WARNING "Not implemented")
endif()

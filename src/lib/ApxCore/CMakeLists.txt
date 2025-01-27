list(APPEND data "#") # mark cmake data format
list(APPEND data "version:${PROJECT_VERSION}")

list(APPEND data "branch:${APX_GIT_BRANCH}")
list(APPEND data "hash:${APX_GIT_HASH}")
list(APPEND data "time:${APX_GIT_TIME}")
list(APPEND data "year:${APX_GIT_YEAR}")

file(RELATIVE_PATH data_path ${CMAKE_CURRENT_SOURCE_DIR}/${APX_INSTALL_BIN_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${APX_INSTALL_DATA_DIR})
file(RELATIVE_PATH libs_path ${CMAKE_CURRENT_SOURCE_DIR}/${APX_INSTALL_BIN_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${APX_INSTALL_LIBS_DIR})
file(RELATIVE_PATH plugins_path ${CMAKE_CURRENT_SOURCE_DIR}/${APX_INSTALL_BIN_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${APX_INSTALL_PLUGINS_DIR})

list(APPEND data "data_path:${data_path}")
list(APPEND data "libs_path:${libs_path}")
list(APPEND data "plugins_path:${plugins_path}")

apx_gensrc(gensrc_app DATA "${data}")

apx_lib(
    SRCS
    "**/*.[ch]*"
    GENSRC
    gensrc_app
    INCLUDES
    .
    DEPENDS
    shared.crc
    shared.xbus
    shared.xbus.serial
    shared.xbus.tcp
    shared.xbus.telemetry
    shared.mandala
    QT
    Core
    Qml
    Quick
    OpenGL
    Widgets
    QuickControls2
    Svg
    Positioning
)

apx_qrc(
    ${MODULE}
    BASE
    "${APX_RESOURCES_DIR}"
    SRCS
    "fonts/Apx*.ttf"
    "fonts/Roboto*.ttf"
    "styles/style-dark.css"
    "icons/material-icons.*"
    "icons/uavos-logo.*"
)

# update mandala UID resources
set(mandala_uid ${APX_RESOURCES_DIR}/scripts/include/MandalaUid.hpp)
get_target_property(gensrc_dir mandala_headers INCLUDE_DIRECTORIES)
set(mandala_uid_src ${gensrc_dir}/MandalaUid.hpp)
add_custom_command(
    OUTPUT ${mandala_uid}
    DEPENDS shared.mandala ${mandala_uid_src}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${mandala_uid_src} ${mandala_uid}
)
add_custom_target(${MODULE}.mandala_uid ALL DEPENDS ${mandala_uid})
add_dependencies(${MODULE} ${MODULE}.mandala_uid)

# install scripts resources
apx_install_res(${APX_RESOURCES_DIR} scripts)

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
    QRC
    "${APX_RESOURCES_DIR}/fonts/Apx*.ttf"
    "${APX_RESOURCES_DIR}/fonts/FreeMono*.ttf"
    "${APX_RESOURCES_DIR}/fonts/Ubuntu-C.ttf"
    "${APX_RESOURCES_DIR}/fonts/Bierahinia.ttf"
    "${APX_RESOURCES_DIR}/styles/style-dark.css"
    "${APX_RESOURCES_DIR}/icons/material-icons.*"
    "${APX_RESOURCES_DIR}/icons/uavos-logo.*"
    INCLUDES
    .
    DEPENDS
    shared.common
    shared.crc
    shared.xbus
    shared.xbus.uart
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
)

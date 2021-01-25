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

apx_qrc(
    ${MODULE}
    BASE
    "${APX_RESOURCES_DIR}"
    SRCS
    "fonts/Apx*.ttf"
    "fonts/FreeMono*.ttf"
    "fonts/Ubuntu-C.ttf"
    "fonts/Bierahinia.ttf"
    "styles/style-dark.css"
    "icons/material-icons.*"
    "icons/uavos-logo.*"
)

apx_install_res(${APX_RESOURCES_DIR} scripts)
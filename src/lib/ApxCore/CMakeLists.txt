list(APPEND data "#") # mark cmake data format
list(APPEND data "version:${PROJECT_VERSION}")

list(APPEND data "branch:${APX_GIT_BRANCH}")
list(APPEND data "hash:${APX_GIT_HASH}")
list(APPEND data "time:${APX_GIT_TIME}")
list(APPEND data "year:${APX_GIT_YEAR}")

list(APPEND data "data_path:${APX_APP_DATA_DIR}")
list(APPEND data "libs_path:${APX_APP_LIBS_DIR}")
list(APPEND data "plugins_path:${APX_APP_PLUGINS_DIR}")

apx_gensrc(gensrc_app DATA "${data}")

apx_gcs_lib(
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
    Gui
    Quick
    OpenGL
    Widgets
    QuickControls2
    Svg
)

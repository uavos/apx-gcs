apx_gcs_app(
    ${PROJECT_NAME}
    SRCS
    "main.cpp"
    DEPENDS
    lib.ApxCore
    QT
    Core
    # Qml
    Gui
    # Quick
    OpenGL
    Widgets
    # QuickControls2
    # Svg
)

apx_gcs_bundle(${PROJECT_NAME} ${PROJECT_DESCRIPTION})

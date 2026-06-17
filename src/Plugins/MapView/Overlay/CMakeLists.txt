apx_plugin(QT
    Core
    Gui
    Network
    Core5Compat
    Qml
    Quick
    QuickControls2
    Svg
    Positioning
)

target_sources(${MODULE} PRIVATE
    OverlayPlugin.h

    Overlay.cpp
    Overlay.h

    OverlayTileModel.cpp
    OverlayTileModel.h

    OverlayTileServer.cpp
    OverlayTileServer.h
)

set_target_properties(${MODULE} PROPERTIES
    OUTPUT_NAME "overlay"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui")
    add_custom_command(TARGET ${MODULE} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${APX_PLUGINS_OUTPUT_DIRECTORY}/overlay-ui"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/ui"
            "${APX_PLUGINS_OUTPUT_DIRECTORY}/overlay-ui"
        COMMENT "Copying Overlay UI files next to plugin"
    )
endif()

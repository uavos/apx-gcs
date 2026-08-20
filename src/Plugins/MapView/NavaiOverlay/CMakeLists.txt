apx_plugin(QT
    Core
    Gui
    Network
    Qml
    Quick
    QuickControls2
    Positioning
)

target_sources(${MODULE} PRIVATE
    NavaiOverlayPlugin.h
    NavaiOverlay.cpp
    NavaiOverlay.h
    NavaiTileServer.cpp
    NavaiTileServer.h
)

set_target_properties(${MODULE} PROPERTIES
    OUTPUT_NAME "navai-overlay"
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui")
    add_custom_command(TARGET ${MODULE} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${APX_PLUGINS_OUTPUT_DIRECTORY}/navai-overlay-ui"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/ui"
            "${APX_PLUGINS_OUTPUT_DIRECTORY}/navai-overlay-ui"
        COMMENT "Copying Navai Overlay UI files next to plugin"
    )
endif()

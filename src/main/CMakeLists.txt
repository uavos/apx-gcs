set(target ${PROJECT_NAME})

set(SRCS "main.cpp" "qml")
add_executable(${target} ${SRCS})

set(QT
    Core
    # Qml
    Gui
    # Quick
    OpenGL
    Widgets
    # QuickControls2
    # Svg
)
apx_qt(${target} ${QT})

apx_qrc(${target} SRCS "qml")

# dependencies

set(DEPENDS lib.ApxGcs)

foreach(dep ${DEPENDS})
    if(NOT TARGET ${dep})
        apx_use_module(${dep})
    endif()
    target_link_libraries(${target} PRIVATE ${dep})
endforeach()

# install
install(TARGETS ${target} RUNTIME DESTINATION ${APX_INSTALL_BIN_DIR})

apx_install_res(${APX_RESOURCES_DIR} vpn)

if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_DESCRIPTION})
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_COPYRIGHT ${APX_COPYRIGHT})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER com.uavos.apx.${target})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_EXECUTABLE_NAME ${target})
    set(MACOSX_BUNDLE_ICON_FILE /icons/uavos-logo.icns)
    configure_file(${APX_RESOURCES_DIR}/macos/Info.plist.in Info.plist)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Info.plist DESTINATION ${APX_INSTALL_BIN_DIR}/..)
    apx_install_res(${APX_RESOURCES_DIR} icons/*.icns)

elseif(UNIX AND NOT APPLE)
    apx_install_res(${APX_RESOURCES_DIR}/linux .)

elseif(WIN32)
    message(FATAL_ERROR "Not implemented")
endif()

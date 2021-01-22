set(target ${PROJECT_NAME})

apx_glob_srcs("main.cpp" "qml")
add_executable(${target} ${SRCS})

apx_qrc(${target} BASE "qml" SRCS "**")

# app meta data
apx_metadata(app.name ${PROJECT_DESCRIPTION})
apx_metadata(app.version ${PROJECT_VERSION})
apx_metadata(app.runtime ${PROJECT_NAME})
apx_metadata(app.platform ${CMAKE_SYSTEM_NAME})
apx_metadata(app.arch ${CMAKE_SYSTEM_PROCESSOR})
apx_metadata(app.build ${CMAKE_BUILD_TYPE})

apx_metadata(app.path.bundle ${APX_INSTALL_APP_DIR})
apx_metadata(app.path.bin ${APX_INSTALL_BIN_DIR})
apx_metadata(app.path.libs ${APX_INSTALL_LIBS_DIR})
apx_metadata(app.path.plugins ${APX_INSTALL_PLUGINS_DIR})
apx_metadata(app.path.data ${APX_INSTALL_DATA_DIR})
apx_metadata(app.path.qt ${QT_DIR})
apx_metadata(app.path.src ${PROJECT_SOURCE_DIR})

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
    apx_install_res(${APX_RESOURCES_DIR}/linux **)

elseif(WIN32)
    message(FATAL_ERROR "Not implemented")
endif()

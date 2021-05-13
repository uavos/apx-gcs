set(target ${PROJECT_NAME})

apx_glob_srcs("*.[ch]*" "qml")

add_executable(${target} ${SRCS})

apx_qrc(${target} BASE "qml" SRCS "**")

# dependencies

set(DEPENDS lib.ApxGcs)

foreach(dep ${DEPENDS})
    if(NOT TARGET ${dep})
        apx_use_module(${dep})
    endif()
    target_link_libraries(${target} PRIVATE ${dep})
endforeach()

# install
apx_install(${target})

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

    apx_metadata(app.icon ${MACOSX_BUNDLE_ICON_FILE})

elseif(UNIX AND NOT APPLE)

    set(SRCS ${APX_RESOURCES_DIR}/linux/**)
    apx_glob_srcs(${SRCS})

    foreach(src ${SRCS})
        file(RELATIVE_PATH dir ${APX_RESOURCES_DIR}/linux ${CMAKE_CURRENT_SOURCE_DIR}/${src})
        get_filename_component(dir ${dir} DIRECTORY)
        install(FILES ${src} DESTINATION ${APX_INSTALL_APP_DIR}/${dir})
    endforeach()

elseif(WIN32)
    message(WARNING "Not implemented")
endif()

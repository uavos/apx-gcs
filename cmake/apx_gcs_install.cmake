if(APPLE)
    set(prefix "${PROJECT_DESCRIPTION}.app/Contents")
    set(APX_INSTALL_BIN_DIR "${prefix}/MacOS")
    set(APX_INSTALL_LIBS_DIR "${prefix}/Frameworks")
    set(APX_INSTALL_PLUGINS_DIR "${prefix}/PlugIns")
    set(APX_INSTALL_DATA_DIR "${prefix}/Resources")

elseif(UNIX AND NOT APPLE)
    set(prefix "usr")
    set(APX_INSTALL_BIN_DIR "${prefix}/bin")
    set(APX_INSTALL_LIBS_DIR "${prefix}/lib/${PROJECT_NAME}")
    set(APX_INSTALL_PLUGINS_DIR "${prefix}/lib/${PROJECT_NAME}/plugins")
    set(APX_INSTALL_DATA_DIR "${prefix}/share/${PROJECT_NAME}")

elseif(WIN32)
    message(FATAL_ERROR "Not implemented")
endif()

function(apx_install)
    install(
        TARGETS ${ARGN}
        # BUNDLE DESTINATION . COMPONENT Runtime
        # RESOURCE DESTINATION .
        RUNTIME DESTINATION ${APX_INSTALL_BIN_DIR} COMPONENT Runtime
        LIBRARY DESTINATION ${APX_INSTALL_LIBS_DIR} COMPONENT Runtime
        FRAMEWORK DESTINATION ${APX_INSTALL_LIB_DIR} COMPONENT Runtime
    )
endfunction()

function(apx_install_res prefix)

    set(SRCS)

    foreach(src ${ARGN})
        list(APPEND SRCS ${prefix}/${src})
    endforeach()

    apx_srcs(SRCS)

    foreach(src ${SRCS})
        file(RELATIVE_PATH dir ${prefix} ${CMAKE_CURRENT_SOURCE_DIR}/${src})
        get_filename_component(dir ${dir} DIRECTORY)
        install(FILES ${src} DESTINATION ${APX_INSTALL_DATA_DIR}/${dir})
    endforeach()

endfunction()

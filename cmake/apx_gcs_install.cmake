if(APPLE)
    set(APX_INSTALL_APP_DIR "${PROJECT_DESCRIPTION}.app")
    set(prefix "${APX_INSTALL_APP_DIR}/Contents")
    set(APX_INSTALL_BIN_DIR "${prefix}/MacOS")
    set(APX_INSTALL_LIBS_DIR "${prefix}/Frameworks")
    set(APX_INSTALL_PLUGINS_DIR "${prefix}/PlugIns/gcs")
    set(APX_INSTALL_DATA_DIR "${prefix}/Resources")

    set(APX_INSTALL_PLUGINS_RPATH "@loader_path/../../Frameworks")
    set(APX_INSTALL_RPATH "@executable_path/../Frameworks")

elseif(UNIX AND NOT APPLE)
    set(APX_INSTALL_APP_DIR "usr")
    set(prefix "${APX_INSTALL_APP_DIR}")
    set(APX_INSTALL_BIN_DIR "${prefix}/bin")
    set(APX_INSTALL_LIBS_DIR "${prefix}/lib")
    set(APX_INSTALL_PLUGINS_DIR "${prefix}/share/${PROJECT_NAME}/plugins")
    set(APX_INSTALL_DATA_DIR "${prefix}/share/${PROJECT_NAME}")

    set(APX_INSTALL_RPATH "$ORIGIN;$ORIGIN/..;$ORIGIN/../lib")
    set(APX_INSTALL_PLUGINS_RPATH "$ORIGIN/../../../lib")

elseif(WIN32)
    message(WARNING "Not implemented")
endif()

function(apx_install)
    install(
        TARGETS ${ARGN}
        # BUNDLE DESTINATION . COMPONENT Runtime
        # RESOURCE DESTINATION .
        RUNTIME DESTINATION ${APX_INSTALL_BIN_DIR} COMPONENT Runtime
        LIBRARY DESTINATION ${APX_INSTALL_LIBS_DIR} COMPONENT Runtime
        FRAMEWORK DESTINATION ${APX_INSTALL_LIBS_DIR} COMPONENT Runtime
    )
    # message(STATUS "INSTALL: ${ARGN}")
    set_target_properties(${ARGN} PROPERTIES INSTALL_RPATH "${APX_INSTALL_RPATH}")
endfunction()

function(apx_install_plugin)
    set(dest ${APX_INSTALL_PLUGINS_DIR})
    install(
        TARGETS ${ARGN}
        BUNDLE DESTINATION ${dest} COMPONENT Plugin
        RUNTIME DESTINATION ${dest} COMPONENT Plugin
        LIBRARY DESTINATION ${dest} COMPONENT Plugin
        FRAMEWORK DESTINATION ${dest} COMPONENT Plugin
    )

    set_target_properties(${ARGN} PROPERTIES INSTALL_RPATH "${APX_INSTALL_PLUGINS_RPATH}")
endfunction()

function(apx_install_res prefix)
    set(SRCS)

    foreach(src ${ARGN})
        list(APPEND SRCS ${prefix}/${src})
    endforeach()

    apx_glob_srcs(SRCS)

    foreach(src ${SRCS})
        file(RELATIVE_PATH dir ${prefix} ${CMAKE_CURRENT_SOURCE_DIR}/${src})
        get_filename_component(dir ${dir} DIRECTORY)
        install(FILES ${src} DESTINATION ${APX_INSTALL_DATA_DIR}/${dir})
    endforeach()

endfunction()

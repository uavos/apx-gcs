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
    set(APX_INSTALL_LIBS_DIR "${prefix}/lib/${PROJECT_NAME}")
    set(APX_INSTALL_PLUGINS_DIR "${prefix}/lib/${PROJECT_NAME}/plugins")
    set(APX_INSTALL_DATA_DIR "${prefix}/share/${PROJECT_NAME}")

    set(APX_INSTALL_PLUGINS_RPATH "\$ORIGIN/../../Frameworks")
    set(APX_INSTALL_RPATH "\$ORIGIN/../Frameworks")

elseif(WIN32)
    message(FATAL_ERROR "Not implemented")
endif()

# set(CMAKE_INSTALL_RPATH "${APX_INSTALL_RPATH}")

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
        BUNDLE DESTINATION ${dest} COMPONENT Runtime
        RUNTIME DESTINATION ${dest} COMPONENT Runtime
        LIBRARY DESTINATION ${dest} COMPONENT Runtime
        FRAMEWORK DESTINATION ${dest} COMPONENT Runtime
    )

    set_target_properties(${ARGN} PROPERTIES INSTALL_RPATH "${APX_INSTALL_PLUGINS_RPATH}")
endfunction()

function(apx_install_res prefix)

    set(SRCS)

    foreach(src ${ARGN})
        list(APPEND SRCS ${prefix}/${src})
    endforeach()

    apx_glob_srcs(${SRCS})

    foreach(src ${SRCS})
        file(RELATIVE_PATH dir ${prefix} ${CMAKE_CURRENT_SOURCE_DIR}/${src})
        get_filename_component(dir ${dir} DIRECTORY)
        install(FILES ${src} DESTINATION ${APX_INSTALL_DATA_DIR}/${dir})
    endforeach()

endfunction()

function(apx_install_bundle_libs)
    install(
        CODE "\n
        message(STATUS \"Deploying app...\")\n
        execute_process(
            WORKING_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}
            OUTPUT_FILE /dev/stdout
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/deploy/deploy_app.py
                --app=\"${APX_INSTALL_APP_DIR}\"
                --meta=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.json
            )"
    )
endfunction()

# set(bundle_file
# add_custom_command(
#     OUTPUT ${json}
#     COMMAND ${PYTHON_EXECUTABLE} ${APX_SHARED_DIR}/tools/gensrc.py --data "${app_meta_data}" --dest ${json}
#     DEPENDS ${APX_SHARED_DIR}/tools/gensrc.py ${target}
#     VERBATIM
# )
# add_custom_target(bundle ALL DEPENDS ${json})

# cmake-format: off
function(apx_gcs_bundle target title)
    apx_parse_function_args(
        NAME apx_gcs_bundle
		ONE_VALUE
            ICON
		ARGN ${ARGN})
# cmake-format: on

    if(NOT APPLE)
        return()
    endif()

    set(copyright "(C) ${APX_GIT_YEAR} Aliaksei Stratsilatau &lt;sa@uavos.com&gt;")

    # bundle
    set_target_properties(
        ${target}
        PROPERTIES #OUTPUT_NAME "${title}"
                   #    RUNTIME_OUTPUT_NAME "${target}"
                   BUNDLE TRUE
                   MACOSX_BUNDLE TRUE
                   MACOSX_BUNDLE_BUNDLE_NAME ${title}
                   MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
                   MACOSX_BUNDLE_COPYRIGHT ${APX_COPYRIGHT}
                   MACOSX_BUNDLE_GUI_IDENTIFIER com.uavos.apx.${target}
                   MACOSX_FRAMEWORK_IDENTIFIER com.uavos.apx.${target}
                   MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
                   MACOSX_BUNDLE_EXECUTABLE_NAME ${target}
    )

    set_target_properties(${target} PROPERTIES BUNDLE TRUE)

    if(ICON)
        set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${ICON})
    endif()

    # # Rename generated executable binary to luminance-hdr
    # add_custom_command(
    #     TARGET ${target}
    #     POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E rename ${title}.app/Contents/MacOS/${title} ${title}.app/Contents/MacOS/${target}
    #     VERBATIM
    #     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    # )

endfunction()

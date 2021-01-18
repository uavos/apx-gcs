# cmake-format: off
function(apx_gcs_bundle target name)
    apx_parse_function_args(
        NAME apx_gcs_bundle
		ONE_VALUE
            ICON
		ARGN ${ARGN})
# cmake-format: on

    set(copyright "(C) ${APX_GIT_YEAR} Aliaksei Stratsilatau &lt;sa@uavos.com&gt;")

    # bundle
    set_target_properties(${target} PROPERTIES BUNDLE TRUE)
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME ${name})
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_COPYRIGHT ${APX_COPYRIGHT})
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER com.uavos.apx.${name})
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})

    if(ICON)
        set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${ICON})
    endif()

endfunction()

# cmake-format: off
function(apx_plugin)
    apx_parse_function_args(
        NAME apx_plugin
		MULTI_VALUE
            SRCS
            DEPENDS
            GENSRC
            QT
        OPTIONS
            QML_NO_PREFIX
		ARGN ${ARGN})
# cmake-format: on

    # guess name from path
    string(REPLACE "/" ";" path_list ${CMAKE_CURRENT_LIST_DIR})
    list(GET path_list -1 target)

    if(NOT SRCS)
        set(SRCS "*.[ch]*")
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/qml)
            list(APPEND SRCS "qml")
        endif()
    endif()
    apx_glob_srcs(${SRCS})

    add_library(${target} MODULE ${SRCS})

    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${target} LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)

    apx_qt(${target} ${QT})

    # depends

    list(APPEND DEPENDS "lib.ApxCore")

    foreach(dep ${DEPENDS})
        if(NOT TARGET ${dep})
            apx_use_module(${dep})
        endif()
        target_link_libraries(${target} PRIVATE ${dep})
    endforeach()

    # definitions
    target_compile_definitions(${target} PRIVATE PLUGIN_NAME=\"${target}\")

    # qml
    if((NOT NO_QML) AND (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/qml))
        if(NOT QML_NO_PREFIX)
            set(qml_prefix ${target})
        endif()
        apx_qrc(
            ${target}
            PREFIX
            ${qml_prefix}
            BASE
            "qml"
            SRCS
            "**"
        )
    endif()

    # set_target_properties(${target} PROPERTIES VERSION ${PROJECT_VERSION})
    # set_target_properties(${target} PROPERTIES SOVERSION 1)

    # bundle
    set_target_properties(
        ${target}
        PROPERTIES BUNDLE TRUE
                   MACOSX_BUNDLE_BUNDLE_NAME ${target}
                   MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
                   MACOSX_BUNDLE_COPYRIGHT ${APX_COPYRIGHT}
                   MACOSX_BUNDLE_GUI_IDENTIFIER com.uavos.apx.${PROJECT_NAME}.${target}
                   MACOSX_FRAMEWORK_IDENTIFIER com.uavos.apx.${PROJECT_NAME}.${target}
                   MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
                   MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
                   MACOSX_BUNDLE_EXECUTABLE_NAME ${target}
                   MACOSX_BUNDLE_INFO_STRING "${target} plugin for ${PROJECT_DESCRIPTION}"
    )

    apx_install_plugin(${target})

    # make name available in parent scope
    set(MODULE
        ${target}
        PARENT_SCOPE
    )

endfunction()

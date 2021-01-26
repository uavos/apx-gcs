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

    add_library(${target} SHARED ${SRCS})

    set_target_properties(
        ${target}
        PROPERTIES APX_PLUGIN TRUE
                   PREFIX ""
                   OUTPUT_NAME ${target}
                   LIBRARY_OUTPUT_DIRECTORY ${APX_PLUGINS_OUTPUT_DIRECTORY}
    )

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

    apx_install_plugin(${target})

    # make name available in parent scope
    set(MODULE
        ${target}
        PARENT_SCOPE
    )

endfunction()

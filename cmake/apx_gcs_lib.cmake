function(apx_lib)
    # cmake-format: off
    apx_parse_function_args(
        NAME apx_gcs_lib
		MULTI_VALUE
            SRCS
            DEPENDS
            GENSRC
            INCLUDES
            QT
		ARGN ${ARGN})
# cmake-format: on

    # add lib as apx_module
    apx_module(
        TYPE
        SHARED
        SRCS
        ${SRCS}
        DEPENDS
        ${DEPENDS}
        GENSRC
        ${GENSRC}
        INCLUDES
        ${INCLUDES}
    )

    apx_qt(${MODULE} ${QT})

    # set_target_properties(${MODULE} PROPERTIES VERSION ${PROJECT_VERSION})
    # set_target_properties(${MODULE} PROPERTIES SOVERSION 1)

    # guess lib name from path
    string(REPLACE "/" ";" path_list ${CMAKE_CURRENT_LIST_DIR})
    list(GET path_list -1 LIB_NAME)
    set_target_properties(${MODULE} PROPERTIES OUTPUT_NAME ${LIB_NAME})

    apx_install(${MODULE})

    get_target_property(lib ${MODULE} OUTPUT_NAME)
    if(APPLE)
        set(prefix "lib")
        set(suffix ".dylib")
    else()
        get_target_property(suffix ${MODULE} SUFFIX)
        get_target_property(prefix ${MODULE} PREFIX)
        if(prefix STREQUAL prefix-NOTFOUND)
            set(prefix)
        endif()
        if(suffix STREQUAL suffix-NOTFOUND)
            set(suffix)
        endif()
    endif()
    set(lib "${prefix}${lib}${suffix}")
    set_property(GLOBAL APPEND PROPERTY APX_LIBS ${lib})

    # make module name available in parent scope
    set(MODULE
        ${MODULE}
        PARENT_SCOPE
    )

endfunction()

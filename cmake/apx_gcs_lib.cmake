# cmake-format: off
function(apx_lib)
    apx_parse_function_args(
        NAME apx_gcs_lib
		MULTI_VALUE
            SRCS
            DEPENDS
            GENSRC
            INCLUDES
            QT
            QRC
            RES
        ONE_VALUE
            QRC_PREFIX
            RES_PREFIX
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

    apx_qrc(${MODULE} PREFIX ${QRC_PREFIX} SRCS ${QRC})

    apx_install(${MODULE})

    if(RES)
        if(NOT RES_PREFIX)
            set(RES_PREFIX ${APX_RESOURCES_DIR})
        endif()
        apx_install_res(${RES_PREFIX} ${RES})
    endif()

    # make module name available in parent scope
    set(MODULE
        ${MODULE}
        PARENT_SCOPE
    )

endfunction()

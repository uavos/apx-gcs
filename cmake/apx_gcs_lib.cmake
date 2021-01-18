# cmake-format: off
function(apx_gcs_lib)
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
        MODULE
        SRCS
        ${SRCS}
        DEPENDS
        ${DEPENDS}
        GENSRC
        ${GENSRC}
        INCLUDES
        ${INCLUDES}
    )

    # qt deps
    if(QT)
        foreach(cmp ${QT})
            find_package(
                Qt5
                COMPONENTS ${cmp}
                REQUIRED
            )
            target_link_libraries(${MODULE} PUBLIC Qt5::${cmp})
        endforeach()
    endif()

    # guess lib name from path
    string(REPLACE "/" ";" path_list ${CMAKE_CURRENT_LIST_DIR})
    list(GET path_list -1 LIB_NAME)

    set_target_properties(${MODULE} PROPERTIES OUTPUT_NAME ${LIB_NAME})

    apx_gcs_bundle(${MODULE} ${LIB_NAME})

endfunction()

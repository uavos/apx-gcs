# cmake-format: off
function(apx_qrc TARGET)
    apx_parse_function_args(
        NAME apx_qrc
        ONE_VALUE
            PREFIX
            BASE
        MULTI_VALUE
            SRCS
		ARGN ${ARGN})
# cmake-format: on

    if(NOT SRCS)
        return()
    endif()

    if(BASE)
        set(srcs)
        foreach(src ${SRCS})
            list(APPEND srcs ${BASE}/${src})
        endforeach()
        set(SRCS ${srcs})
        if(NOT BASE MATCHES "^/")
            set(BASE "${CMAKE_CURRENT_SOURCE_DIR}/${BASE}")
        endif()
    else()
        set(BASE ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    # get_target_property(OUTPUT_NAME ${TARGET} OUTPUT_NAME)

    apx_glob_srcs(${SRCS})

    set(qrc_file "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.qrc")
    file(WRITE ${qrc_file} "<!DOCTYPE RCC><RCC version=\"1.0\">\n")
    file(APPEND ${qrc_file} "<qresource prefix=\"/${PREFIX}\">")
    foreach(qrc_src ${SRCS})
        get_filename_component(qrc_name ${qrc_src} NAME)
        get_filename_component(qrc_src ${qrc_src} ABSOLUTE)
        file(RELATIVE_PATH alias ${BASE} ${qrc_src})
        if(alias MATCHES "\\.\\.+")
            message(FATAL_ERROR "QRC alias: ${alias}")
        endif()
        # set(alias ${qrc_name}) # TODO: relative alias
        file(APPEND ${qrc_file} "\n\t<file alias=\"${alias}\">${qrc_src}</file>")
    endforeach()
    file(APPEND ${qrc_file} "\n</qresource>\n</RCC>")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        qt_add_resources(rcc ${qrc_file} OPTIONS -no-compress)
    else()
        qtquick_compiler_add_resources(rcc ${qrc_file} OPTIONS -no-compress)
    endif()
    target_sources(${TARGET} PRIVATE ${rcc})

endfunction()

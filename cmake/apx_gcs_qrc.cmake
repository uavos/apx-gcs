# cmake-format: off
function(apx_qrc TARGET)
    apx_parse_function_args(
        NAME apx_qrc
        ONE_VALUE
            PREFIX
        MULTI_VALUE
            SRCS
		ARGN ${ARGN})
# cmake-format: on

    if(NOT SRCS)
        return()
    endif()

    # get_target_property(OUTPUT_NAME ${TARGET} OUTPUT_NAME)

    apx_glob_srcs(${SRCS})

    set(qrc_file "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.qrc")
    file(WRITE ${qrc_file} "<!DOCTYPE RCC><RCC version=\"1.0\">\n")
    file(APPEND ${qrc_file} "<qresource prefix=\"/${PREFIX}\">")
    foreach(qrc_src ${SRCS})
        get_filename_component(qrc_name ${qrc_src} NAME)
        file(REAL_PATH ${qrc_src} qrc_src)
        set(alias ${qrc_name}) # TODO: relative alias
        file(APPEND ${qrc_file} "\n\t<file alias=\"${alias}\">${qrc_src}</file>")
    endforeach()
    file(APPEND ${qrc_file} "\n</qresource>\n</RCC>")

    qt_add_resources(rcc ${qrc_file} OPTIONS -no-compress)
    target_sources(${TARGET} PRIVATE ${rcc})

endfunction()

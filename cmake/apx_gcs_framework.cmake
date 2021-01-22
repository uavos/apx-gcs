if(APPLE)
    macro(apx_add_framework target fwname)
        find_library(FW_${fwname} ${fwname} REQUIRED)
        get_filename_component(dir ${FW_${fwname}} DIRECTORY)
        target_link_libraries(${target} PUBLIC "-F ${dir}")
        target_link_libraries(${target} PUBLIC "-framework ${fwname}")
        target_include_directories(${target} PUBLIC "${FW_${fwname}}/Headers")
        message(STATUS "FRAMEWORK: ${fwname} (${dir})")

        set_property(GLOBAL APPEND PROPERTY APX_META_FRAMEWORKS "${FW_${fwname}}")

    endmacro()
endif()

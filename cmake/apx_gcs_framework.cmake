if(APPLE)
    macro(apx_add_framework target fwname)
        # add libs
        find_library(FW_${fwname} ${fwname} REQUIRED)
        get_filename_component(dir ${FW_${fwname}} DIRECTORY)
        target_link_libraries(${target} PUBLIC "-F ${dir}")
        target_link_libraries(${target} PUBLIC "-framework ${fwname}")

        # add includes
        target_compile_options(${target} PUBLIC "-F${dir}")
        # legacy includes
        target_include_directories(${target} PUBLIC "${FW_${fwname}}/Headers")

        # collect all frameworks for deployment
        message(STATUS "FRAMEWORK: ${fwname} (${dir})")
        set_property(GLOBAL APPEND PROPERTY APX_FRAMEWORKS "${FW_${fwname}}")
    endmacro()
endif()

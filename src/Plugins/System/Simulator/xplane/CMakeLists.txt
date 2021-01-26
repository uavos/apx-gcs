apx_module(
    TYPE
    SHARED
    SRCS
    "plugin.cpp"
    "SDK/**/*.[h]*"
    DEPENDS
    shared.xbus
    shared.xbus.tcp
    shared.mandala
)

set_target_properties(
    ${MODULE}
    PROPERTIES OUTPUT_NAME "ApxSIL_${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}"
               PREFIX ""
               SUFFIX ".xpl"
)
install(TARGETS ${MODULE} LIBRARY DESTINATION ${APX_INSTALL_DATA_DIR}/xplane)

target_include_directories(${MODULE} PRIVATE SDK/CHeaders/XPLM)

target_compile_definitions(${MODULE} PRIVATE XPLM200)

target_link_libraries(${MODULE} PRIVATE lib.ApxCore) # for app_data.h

if(APPLE)
    target_compile_definitions(${MODULE} PRIVATE APL=1 IBM=0 LIN=0)
    target_link_options(${MODULE} PRIVATE -Wl,-flat_namespace,-undefined,suppress)

elseif(UNIX AND NOT APPLE)
    target_compile_definitions(${MODULE} PRIVATE APL=0 IBM=0 LIN=1)

elseif(WIN32)
    message(WARNING "Not implemented")
endif()

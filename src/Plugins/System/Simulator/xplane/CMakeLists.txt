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

set(arch ${CMAKE_SYSTEM_PROCESSOR})

if(APPLE)
    set_target_properties(${MODULE} PROPERTIES OSX_ARCHITECTURES "arm64;x86_64")
    set(arch "universal")
endif()

set_target_properties(
    ${MODULE}
    PROPERTIES OUTPUT_NAME "ApxSIL_${CMAKE_SYSTEM_NAME}_${arch}"
               PREFIX ""
               SUFFIX ".xpl"
)
install(TARGETS ${MODULE} LIBRARY DESTINATION ${APX_INSTALL_DATA_DIR}/xplane)

target_include_directories(${MODULE} PRIVATE SDK/CHeaders/XPLM)

target_compile_definitions(${MODULE} PRIVATE XPLM200 VERSION=\"${APX_GIT_VERSION}\")

if(APPLE)
    target_compile_definitions(${MODULE} PRIVATE APL=1 IBM=0 LIN=0)
    target_link_options(${MODULE} PRIVATE -Wl,-flat_namespace,-undefined,suppress)

elseif(UNIX AND NOT APPLE)
    target_compile_definitions(${MODULE} PRIVATE APL=0 IBM=0 LIN=1)
    target_compile_options(shared.xbus.tcp PRIVATE -fPIC)

elseif(WIN32)
    message(WARNING "Not implemented")
endif()

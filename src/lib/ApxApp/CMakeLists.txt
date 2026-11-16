apx_lib(
    SRCS
    "**/*.[ch]*"
    "*.[ch]*"
    INCLUDES
    .
    DEPENDS
    lib.ApxCore
    QT
    Network
    Core5Compat
)

# quazip (bundled, used by the plugins installer and ApxFw)
target_compile_options(${MODULE} PRIVATE -Wno-deprecated-declarations)
target_link_libraries(${MODULE} PUBLIC -lz)

set_property(GLOBAL APPEND PROPERTY APX_EXTLIBS "z")

target_compile_definitions(${MODULE} PRIVATE NOCRYPT NOUNCRYPT)

set_target_properties(${MODULE} PROPERTIES C_STANDARD 99)

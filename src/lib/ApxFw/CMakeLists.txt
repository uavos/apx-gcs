apx_lib(
    SRCS
    "**/*.[ch]*"
    "*.[ch]*"
    INCLUDES
    .
    DEPENDS
    lib.ApxData
    QT
    Core5Compat
)

target_compile_options(${MODULE} PRIVATE -Wno-deprecated-declarations)
target_link_libraries(${MODULE} PUBLIC -lz)

set_property(GLOBAL APPEND PROPERTY APX_EXTLIBS "z")

target_compile_definitions(${MODULE} PRIVATE NOCRYPT)

apx_lib(
    SRCS
    "**/*.[ch]*"
    "*.[ch]*"
    RES
    templates/share
    INCLUDES
    .
    DEPENDS
    lib.ApxData
)

target_compile_options(${MODULE} PRIVATE -Wno-deprecated-declarations)
target_link_libraries(${MODULE} PUBLIC -lz)

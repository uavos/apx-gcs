apx_plugin(DEPENDS lib.ApxGcs)

apx_glob_srcs("qwt/*.[ch]*")
add_library(qwt STATIC ${SRCS})

apx_qt(qwt Concurrent PrintSupport OpenGL Svg)

target_compile_definitions(qwt PRIVATE QWT_NO_SVG)
target_compile_options(qwt PRIVATE -Wno-unused-function -Wno-tautological-constant-out-of-range-compare -Wno-deprecated-declarations)

target_link_libraries(${MODULE} PRIVATE qwt)
target_include_directories(${MODULE} PRIVATE qwt)

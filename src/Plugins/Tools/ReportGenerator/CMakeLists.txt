set(target "ReportGenerator")

apx_plugin(DEPENDS lib.ApxGcs QT 
SRCS "rgTelemetry"
"rgUtils"
".")

target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

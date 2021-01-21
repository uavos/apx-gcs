set(target gcs_plugin_test)
add_executable(${target} "main.cpp")

apx_qt(${target} Core)

install(TARGETS ${target} RUNTIME DESTINATION ${APX_INSTALL_BIN_DIR})

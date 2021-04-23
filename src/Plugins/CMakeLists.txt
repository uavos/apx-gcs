set_property(GLOBAL APPEND PROPERTY APX_PLUGINS_DOCS ${CMAKE_CURRENT_LIST_DIR}/README.md)

add_subdirectory("System")
add_subdirectory("Protocols")
add_subdirectory("MapView")
add_subdirectory("Tools")
add_subdirectory("Windows")

add_subdirectory("PluginTest")

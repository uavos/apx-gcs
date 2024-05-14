apx_plugin(DEPENDS lib.ApxData QT Location)

target_link_libraries(${MODULE} PRIVATE Qt6::LocationPrivate)
target_compile_definitions(${MODULE} PUBLIC QT_STATICPLUGIN)

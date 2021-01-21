apx_plugin(DEPENDS lib.ApxData QT Location)

target_link_libraries(${MODULE} PRIVATE Qt5::LocationPrivate)

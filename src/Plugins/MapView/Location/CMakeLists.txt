set(QT_NO_PRIVATE_MODULE_WARNING ON)

if(Qt6Core_VERSION VERSION_LESS "6.10.0")
    apx_plugin(DEPENDS lib.ApxData QT Location)
    target_link_libraries(${MODULE} PRIVATE Qt6::LocationPrivate)
else()
    apx_plugin(DEPENDS lib.ApxData QT Location LocationPrivate)
endif()

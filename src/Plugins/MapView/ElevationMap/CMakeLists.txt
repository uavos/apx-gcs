apx_plugin(QML_NO_PREFIX DEPENDS lib.ApxGcs QT Location Positioning)

if(APPLE)
    find_package(GDAL CONFIG)
    if(NOT GDAL_FOUND)
        find_package(GDAL REQUIRED)
    endif()
    target_link_libraries(${MODULE} PRIVATE GDAL::GDAL)

elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GDAL REQUIRED gdal)
    target_include_directories(${MODULE} PRIVATE ${GDAL_INCLUDE_DIRS})
    target_link_directories(${MODULE} PRIVATE ${GDAL_LIBRARY_DIRS})
    target_link_libraries(${MODULE} PRIVATE ${GDAL_LIBRARIES})

elseif(WIN32)
    message(WARNING "Not implemented")
endif()

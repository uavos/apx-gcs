apx_plugin(QT Concurrent)

if(APPLE)
    apx_add_framework(${MODULE} SDL2)

elseif(UNIX AND NOT APPLE)
    find_package(SDL2 REQUIRED)
    target_include_directories(${MODULE} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${MODULE} PRIVATE ${SDL2_LIBRARIES})

elseif(WIN32)
    message(FATAL_ERROR "Not implemented")
endif()

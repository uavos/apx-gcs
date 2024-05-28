apx_plugin(QT Concurrent)

apx_install_res(${APX_RESOURCES_DIR} templates/joystick.json)

if(APPLE)
    apx_add_framework(${MODULE} SDL2)

elseif(UNIX AND NOT APPLE)
    # find_package(SDL2 REQUIRED)

    find_file(SDL2_INCLUDE_DIR NAME SDL.h HINTS SDL2)
    find_library(SDL2_LIBRARY NAME SDL2)

    target_include_directories(${MODULE} PRIVATE ${SDL2_INCLUDE_DIRS} "/usr/include/SDL2")
    target_link_libraries(${MODULE} PRIVATE -lSDL2)

elseif(WIN32)
    message(WARNING "Not implemented")
endif()

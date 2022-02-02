apx_plugin(SRCS "*.[ch]*" "${CMAKE_SYSTEM_NAME}" DEPENDS lib.ApxGcs)

target_include_directories(${MODULE} PRIVATE ${CMAKE_SYSTEM_NAME})

if(APPLE)
    apx_add_framework(${MODULE} Sparkle)
    apx_add_framework(${MODULE} AppKit)

elseif(UNIX AND NOT APPLE)
    apx_qrc(
        ${MODULE}
        PREFIX
        ${MODULE}
        BASE
        "Linux"
        SRCS
        "*.qml"
    )

    # AppImageUpdate libs

    target_link_libraries(${MODULE} PRIVATE "-Wl,--whole-archive")
    target_link_libraries(${MODULE} PRIVATE /usr/local/lib/libappimage_shared.a)
    target_link_libraries(${MODULE} PRIVATE /usr/local/lib/libappimage_hashlib.a)

    target_link_libraries(${MODULE} PRIVATE /usr/local/lib/libcpr.a)
    target_link_libraries(${MODULE} PRIVATE /usr/local/lib/librcksum.a)
    target_link_libraries(${MODULE} PRIVATE /usr/local/lib/libzsync2_libz.a)
    target_link_libraries(${MODULE} PRIVATE /usr/local/lib/libzsync2.a)

    target_link_libraries(${MODULE} PRIVATE /usr/local/lib/libappimageupdate.a)

    target_link_libraries(${MODULE} PRIVATE /usr/local/lib/libutil.a)
    target_link_libraries(${MODULE} PRIVATE "-Wl,--no-whole-archive")

    target_link_libraries(${MODULE} PRIVATE -lcurl)

elseif(WIN32)
    message(WARNING "Not implemented")
endif()

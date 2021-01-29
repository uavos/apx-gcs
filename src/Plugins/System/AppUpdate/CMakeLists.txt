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

    target_link_libraries(
        ${MODULE}
        PRIVATE /usr/local/lib/libappimageupdate.a
                # /usr/local/lib/libappimage_shared.a
                # /usr/local/lib/libcpr.a
                # /usr/local/lib/libzsync2.a
                # /usr/local/lib/libzsync.a
                # /usr/local/lib/librcksum.a
                # -lcurl
    )

elseif(WIN32)
    message(WARNING "Not implemented")
endif()

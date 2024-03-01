apx_plugin(DEPENDS lib.ApxGcs QT Multimedia)

if(APPLE)
    apx_add_framework(${MODULE} GStreamer)
    target_link_libraries(${MODULE} PRIVATE "-L${FW_GStreamer}/Versions/Current/lib")

    target_link_options(${MODULE} PRIVATE "-Wl,-rpath,${FW_GStreamer}/Versions/Current/lib")

elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLIB REQUIRED glib-2.0)
    target_include_directories(${MODULE} PRIVATE ${GLIB_INCLUDE_DIRS})
    target_link_directories(${MODULE} PRIVATE ${GLIB_LIBRARY_DIRS})

    set(GSTREAMER_MINIMUM_VERSION 1.0.5)

    pkg_check_modules(GST1_TEST gstreamer-1.0 REQUIRED)

    if(GST1_TEST_FOUND AND NOT ${GST1_TEST_VERSION} VERSION_LESS ${GSTREAMER_MINIMUM_VERSION})
        pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
        pkg_check_modules(GSTREAMER-APP REQUIRED gstreamer-app-1.0)
        pkg_check_modules(GSTREAMER-AUDIO REQUIRED gstreamer-audio-1.0)
        pkg_check_modules(GSTREAMER-PBUTILS REQUIRED gstreamer-pbutils-1.0)
        pkg_check_modules(GSTREAMER-FFT REQUIRED gstreamer-fft-1.0)
        add_definitions(-DGST_API_VERSION_1=1)
    else()
        message(FATAL_ERROR "GST not found (${GST1_TEST_VERSION})")
    endif()

    target_include_directories(
        ${MODULE} PRIVATE ${GSTREAMER_INCLUDE_DIRS} ${GSTREAMER-APP_INCLUDE_DIRS} ${GSTREAMER-AUDIO_INCLUDE_DIRS} ${GSTREAMER-PBUTILS_INCLUDE_DIRS} ${GSTREAMER-FFT_INCLUDE_DIRS}
    )
    target_link_directories(
        ${MODULE}
        PRIVATE
        ${GSTREAMER_LIBRARY_DIRS}
        ${GSTREAMER-APP_LIBRARY_DIRS}
        ${GSTREAMER-AUDIO_LIBRARY_DIRS}
        ${GSTREAMER-PBUTILS_LIBRARY_DIRS}
        ${GSTREAMER-FFT_LIBRARY_DIRS}
    )

    target_link_libraries(
        ${MODULE}
        PRIVATE ${GSTREAMER_LIBRARIES}
                ${GSTREAMER-APP_LIBRARIES}
                ${GSTREAMER-AUDIO_LIBRARIES}
                ${GSTREAMER-PBUTILS_LIBRARIES}
                ${GSTREAMER-FFT_LIBRARIES}
                pthread
                ${GLIB_LIBRARIES}
                ${GLIB_GIO_LIBRARIES}
                ${GLIB_GOBJECT_LIBRARIES}
    )

    # "/usr/include/gstreamer-1.0",
    # "/usr/include/glib-2.0",
    # "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/include",
    # "/usr/lib/x86_64-linux-gnu/glib-2.0/include"

    # "gstapp-1.0", "gstrtp-1.0", "gstbase-1.0", "gstreamer-1.0", "gobject-2.0", "glib-2.0"

elseif(WIN32)
    message(WARNING "Not implemented")
endif()

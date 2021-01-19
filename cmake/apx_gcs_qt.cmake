function(apx_gcs_qt TARGET)

    set(QT ${ARGN})

    if(NOT QT)
        return()
    endif()

    find_package(
        Qt5
        COMPONENTS ${QT}
        REQUIRED
    )
    foreach(cmp ${QT})
        target_link_libraries(${TARGET} PUBLIC Qt5::${cmp})
    endforeach()

endfunction()

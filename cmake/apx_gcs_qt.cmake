function(apx_qt TARGET)

    set(QT ${ARGN})

    if(NOT QT)
        return()
    endif()

    find_package(
        Qt6
        COMPONENTS ${QT}
        REQUIRED
    )
    foreach(cmp ${QT})
        target_link_libraries(${TARGET} PUBLIC Qt6::${cmp})
    endforeach()

endfunction()

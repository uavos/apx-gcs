function(apx_metadata var value)

    set_property(
        TARGET ${PROJECT_NAME}
        APPEND
        PROPERTY APX_META_DATA "${var}:${value}"
    )

endfunction()

function(apx_metadata_generate)

    apx_metadata(app.name ${PROJECT_DESCRIPTION})
    apx_metadata(app.version ${PROJECT_VERSION})
    apx_metadata(app.runtime ${PROJECT_NAME})
    apx_metadata(app.platform ${CMAKE_SYSTEM_NAME})
    apx_metadata(app.arch ${CMAKE_SYSTEM_PROCESSOR})
    apx_metadata(app.build ${CMAKE_BUILD_TYPE})

    apx_metadata(app.path.bundle ${APX_INSTALL_APP_DIR})
    apx_metadata(app.path.bin ${APX_INSTALL_BIN_DIR})
    apx_metadata(app.path.libs ${APX_INSTALL_LIBS_DIR})
    apx_metadata(app.path.plugins ${APX_INSTALL_PLUGINS_DIR})
    apx_metadata(app.path.data ${APX_INSTALL_DATA_DIR})
    apx_metadata(app.path.qt ${QT_DIR})
    apx_metadata(app.path.src ${PROJECT_SOURCE_DIR})

    set(target ${PROJECT_NAME})
    set(json "${CMAKE_CURRENT_BINARY_DIR}/${target}.json")

    get_target_property(app_meta_data ${target} APX_META_DATA)
    list(PREPEND app_meta_data "#") # mark cmake data format

    get_property(frameworks GLOBAL PROPERTY APX_FRAMEWORKS)
    list(APPEND app_meta_data "frameworks:[${frameworks}]")

    get_property(libs GLOBAL PROPERTY APX_LIBS)
    foreach(lib ${libs})
        list(APPEND app_meta_data "libs:[${libs}]")
    endforeach()

    get_property(extlibs GLOBAL PROPERTY APX_EXTLIBS)
    list(APPEND app_meta_data "extlibs:[${extlibs}]")

    set(qtplugins
        audio
        bearer
        # canbus
        # designer
        # gamepads
        generic
        # geometryloaders
        geoservices
        iconengines
        imageformats
        mediaservice
        platforms
        # platformthemes
        # playlistformats
        position
        printsupport
        # qmltooling
        # renderers
        # renderplugins
        # sceneparsers
        # sensorgestures
        # sensors
        sqldrivers/libqsqlite.dylib
        # styles
        texttospeech
        # webview
    )
    list(APPEND app_meta_data "qtplugins:[${qtplugins}]")

    # executables
    set(executables)
    set(plugins)
    apx_get_all_targets(targets)
    foreach(target ${targets})
        if(NOT TARGET ${target})
            continue()
        endif()

        get_target_property(type ${target} TYPE)
        get_target_property(is_plugin ${target} APX_PLUGIN)

        if(is_plugin)
            list(APPEND plugins "${target}")
        elseif(type STREQUAL EXECUTABLE)
            list(APPEND executables "${target}")
        endif()

    endforeach()
    list(APPEND app_meta_data "executables:[${executables}]")
    list(APPEND app_meta_data "plugins:[${plugins}]")

    add_custom_command(
        OUTPUT ${json}
        COMMAND ${PYTHON_EXECUTABLE} ${APX_SHARED_DIR}/tools/gensrc.py --data "${app_meta_data}" --dest ${json}
        DEPENDS ${APX_SHARED_DIR}/tools/gensrc.py ${target}
        VERBATIM
    )
    add_custom_target(${target}.meta ALL DEPENDS ${json})

endfunction()

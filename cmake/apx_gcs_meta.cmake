function(apx_metadata var value)

    set_property(
        TARGET ${PROJECT_NAME}
        APPEND
        PROPERTY APX_META_DATA "${var}:${value}"
    )

endfunction()

function(apx_metadata_generate)

    # generate json
    set(target ${PROJECT_NAME})
    set(json "${CMAKE_CURRENT_BINARY_DIR}/${target}.json")

    get_target_property(app_meta_data ${target} APX_META_DATA)
    list(PREPEND app_meta_data "#") # mark cmake data format

    get_property(plugins GLOBAL PROPERTY APX_META_PLUGINS)
    list(APPEND app_meta_data "plugins:[${plugins}]")

    get_property(frameworks GLOBAL PROPERTY APX_META_FRAMEWORKS)
    list(APPEND app_meta_data "frameworks:[${frameworks}]")

    get_property(libs GLOBAL PROPERTY APX_META_LIBS)
    list(APPEND app_meta_data "libs:[${libs}]")

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

    add_custom_command(
        OUTPUT ${json}
        COMMAND ${PYTHON_EXECUTABLE} ${APX_SHARED_DIR}/tools/gensrc.py --data "${app_meta_data}" --dest ${json}
        DEPENDS ${APX_SHARED_DIR}/tools/gensrc.py ${target}
        VERBATIM
    )
    add_custom_target(${target}.meta ALL DEPENDS ${json})

endfunction()

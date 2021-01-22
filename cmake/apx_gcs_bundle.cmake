function(apx_install_bundle)
    # cmake-format: off
    apx_parse_function_args(
        NAME apx_install_bundle
		MULTI_VALUE
            PLUGINS
		ARGN ${ARGN})
# cmake-format: on

    # install Qt plugins
    # message(STATUS "${QT_DIR}")

    macro(install_qt_plugin _qt_plugin_name _dest _qt_plugins_var)
        if(TARGET ${_qt_plugin_name})
            get_target_property(_qt_plugin_path "${_qt_plugin_name}" LOCATION)
            if(EXISTS "${_qt_plugin_path}")
                get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
                get_filename_component(_qt_plugin_type "${_qt_plugin_path}" PATH)
                get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)
                set(_qt_plugin_dest "${_dest}/${_qt_plugin_type}")
                install(FILES "${_qt_plugin_path}" DESTINATION "${_qt_plugin_dest}")
                list(APPEND ${_qt_plugins_var} "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${_dest}/${_qt_plugin_file}")
            else()
                message(FATAL_ERROR "QT plugin '${_qt_plugin_name}' not found")
            endif()
        elseif(EXISTS "${QT_DIR}/plugins/${_qt_plugin_name}/")
            install(
                DIRECTORY "${QT_DIR}/plugins/${_qt_plugin_name}"
                DESTINATION "${_dest}"
                USE_SOURCE_PERMISSIONS
            )
            file(
                GLOB libs
                RELATIVE "${QT_DIR}/plugins"
                LIST_DIRECTORIES FALSE
                "${QT_DIR}/plugins/${_qt_plugin_name}/*.dylib"
            )
            foreach(lib ${libs})
                list(APPEND ${_qt_plugins_var} "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${_dest}/${lib}")
            endforeach()
            # message(STATUS "QT_PLUGIN: ${QT_DIR}/plugins/${_qt_plugin_name}")
        elseif(EXISTS "${QT_DIR}/plugins/${_qt_plugin_name}")
            file(RELATIVE_PATH dest "${QT_DIR}/plugins" "${QT_DIR}/plugins/${_qt_plugin_name}")
            get_filename_component(dest ${dest} DIRECTORY)
            set(dest "${_dest}/${dest}")
            install(FILES "${QT_DIR}/plugins/${_qt_plugin_name}" DESTINATION "${dest}")
            list(APPEND ${_qt_plugins_var} "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${_dest}/${_qt_plugin_name}")
            # message(STATUS "QT_PLUGIN: ${QT_DIR}/plugins/${_qt_plugin_name} -> ${dest}")
        else()
            message(FATAL_ERROR "QT plugin folder '${_qt_plugin_name}' not found")
        endif()
    endmacro()

    foreach(plugin ${PLUGINS})
        install_qt_plugin(${plugin} ${APX_INSTALL_PLUGINS_DIR} LIBS)
    endforeach()

    # Qt conf file

    if(APPLE)
        set(prefix_base ${APX_INSTALL_BIN_DIR}/..)
    else()
        set(prefix_base ${APX_INSTALL_BIN_DIR})
    endif()

    set(prefix "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/")
    file(RELATIVE_PATH qt_plugins_dir ${prefix}${prefix_base} ${prefix}${APX_INSTALL_PLUGINS_DIR})
    file(RELATIVE_PATH qt_imports_dir ${prefix}${prefix_base} ${prefix}${APX_INSTALL_DATA_DIR}/qml)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf" "[Paths]\nPlugins = ${qt_plugins_dir}\nImports = ${qt_imports_dir}\nQml2Imports = ${qt_imports_dir}\n")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf" DESTINATION "${APX_INSTALL_DATA_DIR}")

    # Qt QML imports
    install(
        DIRECTORY "${QT_DIR}/qml"
        DESTINATION "${APX_INSTALL_DATA_DIR}"
        USE_SOURCE_PERMISSIONS
    )
    file(
        GLOB_RECURSE libs
        RELATIVE "${QT_DIR}/qml"
        "${QT_DIR}/qml/*.dylib"
    )
    foreach(lib ${libs})
        list(APPEND LIBS "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${APX_INSTALL_DATA_DIR}/qml/${lib}")
    endforeach()

    # scan app plugins
    get_property(plugins GLOBAL PROPERTY APX_PLUGINS)
    foreach(plugin ${plugins})
        get_target_property(lib ${plugin} OUTPUT_NAME)
        get_target_property(suffix ${plugin} SUFFIX)
        get_target_property(prefix ${plugin} PREFIX)
        if(APPLE)
            set(lib "${lib}.bundle/Contents/MacOS/${lib}")
        else()
            if(prefix STREQUAL prefix-NOTFOUND)
                set(prefix)
            endif()
            if(suffix STREQUAL suffix-NOTFOUND)
                set(suffix)
            endif()
            set(lib "${prefix}${lib}${suffix}")
        endif()
        list(APPEND LIBS "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${APX_INSTALL_PLUGINS_DIR}/${lib}")
        # get_target_property(name ${plugin} OUTPUT_NAME)
        # message(STATUS "${name}")
    endforeach()
    # file(
    #     GLOB libs
    #     RELATIVE "${APX_PLUGINS_OUTPUT_DIRECTORY}"
    #     "${APX_PLUGINS_OUTPUT_DIRECTORY}/*.dylib"
    # )
    # foreach(lib ${libs})
    #     list(APPEND LIBS "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${APX_INSTALL_DATA_DIR}/qml/${lib}")
    # endforeach()

    # fix bundle

    set(APPS "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${APX_INSTALL_APP_DIR}")

    # Directories to look for dependencies
    set(DIRS "${CMAKE_BINARY_DIR}")
    list(APPEND DIRS "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    list(APPEND DIRS "${QT_DIR}/lib")

    if(APPLE)
        list(APPEND DIRS ${CMAKE_SYSTEM_FRAMEWORK_PATH})
        list(APPEND DIRS /Library/Frameworks/SDL2.framework/Versions/Current/Frameworks)
    endif()

    # Path used for searching by FIND_XXX(), with appropriate suffixes added
    if(CMAKE_PREFIX_PATH)
        foreach(dir ${CMAKE_PREFIX_PATH})
            list(APPEND DIRS "${dir}/bin" "${dir}/lib")
        endforeach()
    endif()
    list(REMOVE_DUPLICATES DIRS)

    include(InstallRequiredSystemLibraries)

    # message(STATUS "APPS: ${APPS}")
    # message(STATUS "LIBS: ${LIBS}")
    # message(STATUS "DIRS: ${DIRS}")

    install(CODE "include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"${LIBS}\" \"${DIRS}\")"
    )

endfunction()

set(APX_DEPLOY_DIR ${CMAKE_CURRENT_BINARY_DIR}/deploy)

if(CMAKE_BUILD_TYPE STREQUAL Release)
    add_custom_target(
        deploy_bundle
        COMMAND ${CMAKE_COMMAND} -E rm -Rf ${APX_DEPLOY_DIR}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --config Release
        COMMAND ${CMAKE_COMMAND} -E env --unset=DESTDIR ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR} --prefix ${APX_DEPLOY_DIR} --strip
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM USES_TERMINAL
    )
else()
    set(APX_DEPLOY_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/release)
    add_custom_target(
        deploy_bundle
        COMMAND ${CMAKE_COMMAND} -E rm -Rf ${APX_DEPLOY_DIR}
        COMMAND ${CMAKE_COMMAND} -GNinja -B${APX_DEPLOY_BUILD_DIR} -H. -DCMAKE_BUILD_TYPE=Release
        COMMAND ${CMAKE_COMMAND} --build ${APX_DEPLOY_BUILD_DIR} --config Release
        COMMAND ${CMAKE_COMMAND} -E env --unset=DESTDIR ${CMAKE_COMMAND} --install ${APX_DEPLOY_BUILD_DIR} --prefix ${APX_DEPLOY_DIR} --strip
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM USES_TERMINAL
    )
endif()

if(APPLE)
    add_custom_target(
        deploy_qt
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/deploy/deploy_qt.py --app=${APX_DEPLOY_DIR} --meta=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.json
        COMMENT "Deploying Qt..."
        DEPENDS deploy_bundle ${PROJECT_NAME}.meta
        WORKING_DIRECTORY ${APX_DEPLOY_DIR}
        VERBATIM USES_TERMINAL
    )
elseif(UNIX AND NOT APPLE)
    add_custom_target(
        deploy_qt
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/deploy/deploy_qt.py --app=${APX_DEPLOY_DIR} --meta=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.json
        COMMAND ${CMAKE_COMMAND} -E env --unset=DESTDIR ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR} --prefix ${APX_DEPLOY_DIR} --strip --component Plugin
        COMMENT "Deploying Qt..."
        DEPENDS deploy_bundle ${PROJECT_NAME}.meta
        WORKING_DIRECTORY ${APX_DEPLOY_DIR}
        VERBATIM USES_TERMINAL
    )
endif()

add_custom_target(
    deploy_libs
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/deploy/deploy_libs.py --app=${APX_DEPLOY_DIR} --meta=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.json --dist=$ENV{LIBS_DIST_DIR}
    COMMENT "Deploying libs..."
    DEPENDS deploy_qt
    WORKING_DIRECTORY ${APX_DEPLOY_DIR}
    VERBATIM USES_TERMINAL
)

add_custom_target(bundle DEPENDS deploy_libs)

if(APPLE)
    set(codesign_cmd
        codesign
        --deep
        --force
        --options
        runtime
        -s
        $ENV{CODE_IDENTITY}
    )
    add_custom_target(
        deploy_sign
        COMMAND ${codesign_cmd} ${APX_INSTALL_DATA_DIR}/xplane/ApxSIL_Darwin_universal.xpl
        COMMAND ${codesign_cmd} ${APX_INSTALL_BIN_DIR}/gcs
        COMMAND ${codesign_cmd} ${APX_INSTALL_APP_DIR}
        COMMENT "Signing app with $ENV{CODE_IDENTITY}"
        # DEPENDS deploy_libs
        WORKING_DIRECTORY ${APX_DEPLOY_DIR}
        VERBATIM USES_TERMINAL
    )

    add_custom_target(
        deploy_package
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/deploy/deploy_dmg.py --app=${APX_DEPLOY_DIR} --meta=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.json
        COMMENT "Creating DMG..."
        DEPENDS deploy_sign
        WORKING_DIRECTORY ${APX_DEPLOY_DIR}
        VERBATIM USES_TERMINAL
    )

elseif(UNIX AND NOT APPLE)
    add_custom_target(
        deploy_package
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/deploy/deploy_appimage.py --app=${APX_DEPLOY_DIR} --meta=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.json
                --apprun=${CMAKE_CURRENT_SOURCE_DIR}/tools/deploy/AppRun.sh
        COMMENT "Creating AppImage..."
        DEPENDS deploy_libs
        WORKING_DIRECTORY ${APX_DEPLOY_DIR}
        VERBATIM USES_TERMINAL
    )

elseif(WIN32)
    message(WARNING "Not implemented")
endif()

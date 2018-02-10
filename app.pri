#
# OS Specific Application settings
#

QMAKE_POST_LINK += echo "Copying files"

MacBuild {
    QMAKE_INFO_PLIST    = Info.plist
    ICON                = $$RES_DIR/icons/uavos-logo.icns
    OTHER_FILES        += Info.plist
    LIBS               += -framework ApplicationServices
}

iOSBuild {
    LIBS               += -framework AVFoundation
    #-- Info.plist (need an "official" one for the App Store)
    ForAppStore {
        message(App Store Build)
        #-- Create official, versioned Info.plist
        APP_STORE = $$system(cd $${BASEDIR} && $${BASEDIR}/tools/update_ios_version.sh $${BASEDIR}/ios/iOSForAppStore-Info-Source.plist $${BASEDIR}/ios/iOSForAppStore-Info.plist)
        APP_ERROR = $$find(APP_STORE, "Error")
        count(APP_ERROR, 1) {
            error("Error building .plist file. 'ForAppStore' builds are only possible through the official build system.")
        }
        QT               += qml-private
        CONFIG           += qtquickcompiler
        QMAKE_INFO_PLIST  = $${BASEDIR}/ios/iOSForAppStore-Info.plist
        OTHER_FILES      += $${BASEDIR}/ios/iOSForAppStore-Info.plist
    } else {
        QMAKE_INFO_PLIST  = $${BASEDIR}/ios/iOS-Info.plist
        OTHER_FILES      += $${BASEDIR}/ios/iOS-Info.plist
    }
    BUNDLE.files        = $$files($$PWD/ios/AppIcon*.png) $$PWD/ios/LaunchScreen.xib $$QMAKE_INFO_PLIST
    QMAKE_BUNDLE_DATA  += BUNDLE
    #-- TODO: Add iTunesArtwork
}

LinuxBuild {
    CONFIG  += qesp_linux_udev
}

WindowsBuild {
    RC_ICONS = resources/icons/uavos-logo.ico
}


#
# OS Specific files copy and fix
#

#
# Copy the application resources to the associated place alongside the application
#

LinuxBuild {
    DESTDIR_COPY_RESOURCE_LIST = $$DESTDIR
}

MacBuild {
    DESTDIR_COPY_RESOURCE_LIST = $$DESTDIR/$${TARGET}.app/Contents/MacOS
}


#
# Perform platform specific setup
#

MacBuild {
    # Update version info in bundle
    QMAKE_POST_LINK += && /usr/libexec/PlistBuddy -c \"Set :CFBundleShortVersionString $${MAC_VERSION}\" $$DESTDIR/$${TARGET}.app/Contents/Info.plist
    QMAKE_POST_LINK += && /usr/libexec/PlistBuddy -c \"Set :CFBundleVersion $${MAC_BUILD}\" $$DESTDIR/$${TARGET}.app/Contents/Info.plist
}

MacBuild {
    # Copy non-standard frameworks into app package
    #QMAKE_POST_LINK += && rsync -a --delete $$BASEDIR/libs/lib/Frameworks $$DESTDIR/$${TARGET}.app/Contents/
    # SDL2 Framework
    #QMAKE_POST_LINK += && install_name_tool -change "@rpath/SDL2.framework/Versions/A/SDL2" "@executable_path/../Frameworks/SDL2.framework/Versions/A/SDL2" $$DESTDIR/$${TARGET}.app/Contents/MacOS/$${TARGET}
}

WindowsBuild {
    BASEDIR_WIN = $$replace(BASEDIR, "/", "\\")
    DESTDIR_WIN = $$replace(DESTDIR, "/", "\\")
    QT_BIN_DIR  = $$dirname(QMAKE_QMAKE)

    # Copy dependencies
    DebugBuild: DLL_QT_DEBUGCHAR = "d"
    ReleaseBuild: DLL_QT_DEBUGCHAR = ""
    COPY_FILE_LIST = \
        $$BASEDIR\\libs\\lib\\sdl2\\msvc\\lib\\x86\\SDL2.dll \
        $$BASEDIR\\deploy\\libeay32.dll

    for(COPY_FILE, COPY_FILE_LIST) {
        QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"$$COPY_FILE\" \"$$DESTDIR_WIN\"
    }

    ReleaseBuild {
        # Copy Visual Studio DLLs
        # Note that this is only done for release because the debugging versions of these DLLs cannot be redistributed.
        win32-msvc2010 {
            QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcp100.dll\"  \"$$DESTDIR_WIN\"
            QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcr100.dll\"  \"$$DESTDIR_WIN\"

        } else:win32-msvc2012 {
            QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcp110.dll\"  \"$$DESTDIR_WIN\"
            QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcr110.dll\"  \"$$DESTDIR_WIN\"

        } else:win32-msvc2013 {
            QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcp120.dll\"  \"$$DESTDIR_WIN\"
            QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcr120.dll\"  \"$$DESTDIR_WIN\"

        } else:win32-msvc2015 {
            QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\msvcp140.dll\"  \"$$DESTDIR_WIN\"
            QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY \"C:\\Windows\\System32\\vcruntime140.dll\"  \"$$DESTDIR_WIN\"

        } else {
            error("Visual studio version not supported, installation cannot be completed.")
        }
    }

    DEPLOY_TARGET = $$shell_quote($$shell_path($$DESTDIR_WIN\\$${TARGET}.exe))
    QMAKE_POST_LINK += $$escape_expand(\\n) $$QT_BIN_DIR\\windeployqt --no-compiler-runtime --qmldir=$${BASEDIR_WIN}\\src $${DEPLOY_TARGET}
}

LinuxBuild {
    QMAKE_POST_LINK += && mkdir -p $$DESTDIR/Qt/libs && mkdir -p $$DESTDIR/Qt/plugins

    # QT_INSTALL_LIBS
    QT_LIB_LIST = \
        libQt5Core.so.5 \
        libQt5DBus.so.5 \
        libQt5Gui.so.5 \
        libQt5Location.so.5 \
        libQt5Multimedia.so.5 \
        libQt5MultimediaQuick_p.so.5 \
        libQt5Network.so.5 \
        libQt5OpenGL.so.5 \
        libQt5Positioning.so.5 \
        libQt5PrintSupport.so.5 \
        libQt5Qml.so.5 \
        libQt5Quick.so.5 \
        libQt5QuickControls2.so.5 \
        libQt5QuickTemplates2.so.5 \
        libQt5QuickWidgets.so.5 \
        libQt5SerialPort.so.5 \
        libQt5Sql.so.5 \
        libQt5Svg.so.5 \
        libQt5Test.so.5 \
        libQt5Widgets.so.5 \
        libQt5XcbQpa.so.5 \
        libQt5Xml.so.5 \
        libQt5TextToSpeech.so.5

    !contains(DEFINES, __rasp_pi2__) {
        QT_LIB_LIST += \
            libicudata.so.56 \
            libicui18n.so.56 \
            libicuuc.so.56
    }

    for(QT_LIB, QT_LIB_LIST) {
        QMAKE_POST_LINK += && $$QMAKE_COPY --dereference $$[QT_INSTALL_LIBS]/$$QT_LIB $$DESTDIR/Qt/libs/
    }

    # QT_INSTALL_PLUGINS
    QT_PLUGIN_LIST = \
        bearer \
        geoservices \
        iconengines \
        imageformats \
        platforminputcontexts \
        platforms \
        position \
        sqldrivers \
        texttospeech

    !contains(DEFINES, __rasp_pi2__) {
        QT_PLUGIN_LIST += xcbglintegrations
    }

    for(QT_PLUGIN, QT_PLUGIN_LIST) {
        QMAKE_POST_LINK += && $$QMAKE_COPY --dereference --recursive $$[QT_INSTALL_PLUGINS]/$$QT_PLUGIN $$DESTDIR/Qt/plugins/
    }

    # QT_INSTALL_QML
    QMAKE_POST_LINK += && $$QMAKE_COPY --dereference --recursive $$[QT_INSTALL_QML] $$DESTDIR/Qt/

    # QGroundControl start script
    QMAKE_POST_LINK += && $$QMAKE_COPY $$BASEDIR/deploy/qgroundcontrol-start.sh $$DESTDIR
    QMAKE_POST_LINK += && $$QMAKE_COPY $$BASEDIR/deploy/qgroundcontrol.desktop $$DESTDIR
    QMAKE_POST_LINK += && $$QMAKE_COPY $$BASEDIR/resources/icons/qgroundcontrol.png $$DESTDIR
}


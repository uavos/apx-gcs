
#QMAKE_LFLAGS += -Wl,--rpath=\\\$\$ORIGIN
unix {
  QMAKE_RPATHDIR += $ORIGIN/../lib
  QMAKE_RPATHDIR += $ORIGIN/../Qt

  isEmpty(PREFIX) {
    PREFIX = /usr
  }

  PREFIX = $$INSTALL_ROOT/$$PREFIX
  INSTALLBASE_LIB = $$PREFIX/lib/uavos
  INSTALLBASE_RES = $$PREFIX/share/uavos

}

!hpux:!mac {
  QMAKE_COPY_FILE = $${QMAKE_COPY_FILE} -P -p timestamps
}

# platform dependent install
android {
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/src/android

    DISTFILES += \
        AndroidManifest.xml \
        ../android/gradle/wrapper/gradle-wrapper.jar \
        ../android/gradlew \
        ../android/res/values/libs.xml \
        ../android/build.gradle \
        ../android/gradle/wrapper/gradle-wrapper.properties \
        ../android/gradlew.bat

    assets.path = /assets
    assets.files += ../../assets
    INSTALLS += assets

    x86 {
        target.path = /libs/x86
    } else: armeabi-v7a {
        target.path = /libs/armeabi-v7a
    } else {
        target.path = /libs/armeabi
    }
    export(target.path)
    INSTALLS += target

#}else:mac {
}else:unix {

  plugin {
    target.path = $$INSTALLBASE_LIB/plugins/gcu
    qtlibs.extra = ../../copy-libs.sh $$DESTDIR/lib$${TARGET}.so $$INSTALLBASE_LIB/Qt/lib
  } else {
    target.path = $$INSTALLBASE_LIB
    launcher.path = $$INSTALLBASE_LIB
    launcher.files = ../shared/qt.conf
    launcher.extra = mkdir -p $$PREFIX/bin && cp -af ../../launcher.sh $$PREFIX/bin/$$TARGET
    !uavos-data: INSTALLS += launcher
    qtlibs.extra = ../../copy-libs.sh $$DESTDIR/$$TARGET $$INSTALLBASE_LIB/Qt/lib
  }


  # Qt libraries
  qtlibs.path = $$INSTALLBASE_LIB/Qt/lib

  uavos-data {
    INSTALLS += qtlibs
  } else {
    INSTALLS += target
  }
}

export(INSTALLS)


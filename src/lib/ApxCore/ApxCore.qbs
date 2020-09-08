import qbs
import qbs.File
import qbs.FileInfo
import ApxApp

ApxApp.ApxLibrary {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "qml",
            "gui",
            "quick",
            "opengl",
            "widgets",
            "quickcontrols2",
            "svg",
        ]
    }

    Depends { name: "version_hpp" }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.sourceDirectory
    }

    Depends { name: "cpp" }
    Depends { name: "sdk"; submodules: [ "libs", "headers" ] }

    cpp.defines: base.concat([
        'RELATIVE_PLUGIN_PATH="' + FileInfo.relativePath('/' + app.app_bin_path, '/' + app.app_plugin_path) + '"',
        'RELATIVE_DATA_PATH="' + FileInfo.relativePath('/' + app.app_bin_path, '/' + app.app_data_path) + '"',
        'RELATIVE_LIB_PATH="' + FileInfo.relativePath('/' + app.app_bin_path, '/' + app.app_library_path) + '"',
    ])

    cpp.includePaths: [
        product.sourceDirectory,
        "App/qtsingleapplication",
        "App/qtlockedfile",
    ]

    //force headers bundle creation
    //bundle.publicHeaders: product.sourceDirectory+"/"+"ApxCore.h"


    Group {
        name: "Fact"
        prefix: name+"/"
        files: [
            "Fact.cpp", "Fact.h",
            "FactData.cpp", "FactData.h",
            "FactListModel.cpp", "FactListModel.h",
            "FactListModelActions.cpp", "FactListModelActions.h",
            "FactBase.cpp", "FactBase.h",
            "FactPropertyBinding.cpp", "FactPropertyBinding.h",
        ]
    }

    Group {
        name: "App"
        prefix: name+"/"
        files: [
            "PluginInterface.h",
            "App.cpp", "App.h",
            "AppBase.cpp", "AppBase.h",
            "AppRoot.cpp", "AppRoot.h",
            "AppEngine.cpp", "AppEngine.h",
            "AppPlugins.cpp", "AppPlugins.h",
            "AppPlugin.cpp", "AppPlugin.h",
            "AppSettings.cpp", "AppSettings.h",
            "AppWindow.cpp", "AppWindow.h",
            "AppQuickView.cpp", "AppQuickView.h",
            "AppInstances.cpp", "AppInstances.h",
            "AppLog.cpp", "AppLog.h",
            "AppDirs.cpp", "AppDirs.h",
            "AppNotify.cpp", "AppNotify.h",
            "AppNotifyListModel.cpp", "AppNotifyListModel.h",
            "AppPrefs.cpp", "AppPrefs.h",
        ]
    }

    Group {
        name: "ApxMisc"
        prefix: name+"/"
        files: [
            "QueueJob.cpp", "QueueJob.h",
            "QueueWorker.cpp", "QueueWorker.h",
            "DelayedEvent.cpp", "DelayedEvent.h",
            "FactValue.h",
            "SvgImageProvider.cpp", "SvgImageProvider.h",
            "MaterialIcon.cpp", "MaterialIcon.h",
            "QActionFact.cpp", "QActionFact.h",
            "FactQml.cpp", "FactQml.h",
            "WidgetQmlItem.cpp", "WidgetQmlItem.h",
        ]
    }

    files: [
    ]

    Group {
        name: "Resources.qrc"
        prefix: project.resorcesDir+"/"
        fileTags: "qt.core.resource_data"
        Qt.core.resourcePrefix: "/"
        Qt.core.resourceSourceBase: prefix
        files: [
            "fonts/BebasNeue.otf",
            "fonts/FreeMono*.ttf",
            "fonts/Ubuntu-C.ttf",
            "fonts/Bierahinia.ttf",

            "styles/style-dark.css",

            "icons/material-icons.*",
            "icons/uavos-logo.*",
        ]
    }

    ApxApp.ApxResource {
        src: "scripts"
        files: [
            "*.*",
        ]
    }


}

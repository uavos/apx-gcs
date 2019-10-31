import qbs
import qbs.FileInfo

import ApxApp

ApxApp.ApxProduct {

    targetName: "gcs"

    property path appIcon: "/icons/uavos-logo.icns"

    bundle.bundleName: app.app_display_name+".app"
    infoPlist: ({
                    "CFBundleIconFile": appIcon,
                    "CFBundleDisplayName": app.app_display_name,
                    "CFBundleName": "APX",

                    "SUFeedURL": "https://uavos.github.io/apx-releases/appcast.xml",
                    "SUPublicEDKey": "RoYj8WHIBhxsrYpkUktfoQmroQF91n/jB5pSZ4lwEM4=",
                    "SUEnableAutomaticChecks": true,
                    "SUScheduledCheckInterval": 86400,
                    "SUAllowsAutomaticUpdates": false,
                    "SUAutomaticallyUpdate": false,
    })
    targetInstallDir: bundle.isBundle?app.app_dest_path:app.app_bin_path

    Depends { name: "ApxCore" }
    Depends { name: "ApxGcs" }

    Depends { name: "qmlqrc" }
    qmlqrc.usePrefix: false


    Depends {
        name: "Qt";
        submodules: [
            "core",
            "widgets",
            "xml",
            "quick",
            "quickwidgets",
            "quickcontrols2",
            "svg",
            "sql" ,
            "network" ,
            "serialport" ,
            "multimedia" ,
            "opengl" ,
            "location-private" ,
            "positioning-private" ,
        ]
    }


    //COMPILER
    cpp.rpaths: qbs.targetOS.contains("macos")
            ? [
                  FileInfo.joinPaths("@executable_path", FileInfo.relativePath("/"+app.app_bin_path, "/"+app.app_library_path))
              ]
            : [
                  FileInfo.joinPaths("$ORIGIN", FileInfo.relativePath("/"+app.app_bin_path, "/"+app.app_library_path))
              ]



    files: [
        "main.cpp",
        "app-Info.plist"
    ]

    ApxApp.ApxResource {
        src: "icons"
        files: [
            "*.icns",
        ]
    }

    ApxApp.ApxResource {
        src: "vpn"
        files: [
            "**/*",
        ]
    }

    Group {
        name: "Resources.linux"
        condition: qbs.targetOS.contains("linux")
        qbs.install: true
        qbs.installDir: FileInfo.joinPaths(app.app_data_path,"../")
        qbs.installSourceBase: prefix
        prefix: FileInfo.joinPaths(project.resorcesDir,"linux/")
        files: [
            "**/*",
        ]
    }


}

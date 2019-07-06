import qbs
import qbs.FileInfo

import apx.Application as APX

APX.ApxProduct {

    targetName: "gcs"

    property path appIcon: "/icons/uavos-logo.icns"

    Depends { name: "apx" }
    bundle.bundleName: apx.app_display_name+".app"
    infoPlist: ({
                    "CFBundleIconFile": appIcon,
                    "CFBundleDisplayName": apx.app_display_name,
                    "CFBundleName": "APX",

                    "SUFeedURL": "https://uavos.github.io/apx-releases/appcast.xml",
                    "SUPublicEDKey": "RoYj8WHIBhxsrYpkUktfoQmroQF91n/jB5pSZ4lwEM4=",
                    "SUEnableAutomaticChecks": true,
                    "SUScheduledCheckInterval": 86400,
                    "SUAllowsAutomaticUpdates": false,
                    "SUAutomaticallyUpdate": false,
    })
    targetInstallDir: bundle.isBundle?apx.app_dest_path:apx.app_bin_path

    Depends { name: "ApxCore" }
    Depends { name: "ApxGcs" }

    //Depends { name: "pawncc" }

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
            ? [FileInfo.joinPaths("@executable_path", FileInfo.relativePath("/"+apx.app_bin_path, "/"+apx.app_library_path))]
            : [FileInfo.joinPaths("$ORIGIN", FileInfo.relativePath("/"+apx.app_bin_path, "/"+apx.app_library_path))]




    files: [
        "main.cpp",
        "app-Info.plist"
    ]

    APX.ApxResource {
        name: "icons"
        files: [
            "*.icns",
        ]
    }

    APX.ApxResource {
        name: "vpn"
        files: [
            "**/*",
        ]
    }

    Group {
        name: "linux.res"
        condition: qbs.targetOS.contains("linux")
        qbs.install: true
        qbs.installDir: FileInfo.joinPaths(apx.app_data_path,"../")
        qbs.installSourceBase: prefix
        prefix: FileInfo.joinPaths(project.resorcesDir,"linux/")
        files: [
            "**/*",
        ]
    }


}

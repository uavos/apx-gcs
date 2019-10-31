import qbs.FileInfo

import ApxApp

ApxApp.ApxProduct {

    targetName: "gcs_plugin_test"

    bundle.isBundle: false
    targetInstallDir: app.app_bin_path

    files: [
        "main.cpp",
    ]

    Depends {
        name: "Qt";
        submodules: [
            "core",
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

}

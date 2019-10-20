import qbs
import qbs.FileInfo
import qbs.File

ApxProduct {

    type: ["dynamiclibrary", "dynamiclibrary_symlink"].concat(isForAndroid ? ["android.nativelibrary"] : []);

    targetInstallDir: app.app_library_path

    Depends { name: "cpp" }

    //COMPILER
    Properties {
        condition: qbs.targetOS.contains("darwin")
        cpp.linkerFlags: ["-compatibility_version", cpp.soVersion]
    }

    cpp.rpaths: qbs.targetOS.contains("macos")
            ? [
                  FileInfo.joinPaths("@executable_path", FileInfo.relativePath("/"+app.app_bin_path, "/"+app.app_library_path))]
            : [
                  "$ORIGIN",
                  "$ORIGIN/..",
                  FileInfo.joinPaths("$ORIGIN", FileInfo.relativePath("/"+app.app_bin_path, "/"+app.app_library_path))
              ]


    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [product.sourceDirectory]
    }

}

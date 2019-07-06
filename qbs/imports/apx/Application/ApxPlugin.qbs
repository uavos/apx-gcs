import qbs
import qbs.FileInfo
import qbs.File

ApxProduct {

    type: {
        var v=[]
        if(isForDarwin) v.push("loadablemodule")
        else if(isForLinux){
            v.push("loadablemodule")
        }else{
            v.push("dynamiclibrary")
            if(isForAndroid) v.push("android.nativelibrary")
        }
        return v
    }

    targetInstallDir: apx.app_plugin_path

    Depends { name: "ApxCore" }
    Depends { name: "ApxGcs" }

    Depends { name: "bundle" }
    bundle.identifierPrefix: apx.bundle_identifier+".plugin"

    Properties {
        condition: isForLinux
        cpp.variantSuffix: ".so"
    }

    //COMPILER
    Depends { name: "cpp" }

    cpp.visibility: "minimal"

    cpp.driverLinkerFlags: base.concat(["-rdynamic"])

    cpp.rpaths: qbs.targetOS.contains("macos")
            ? [
                  FileInfo.joinPaths("@loader_path", FileInfo.relativePath("/"+apx.app_bin_path, "/"+apx.app_library_path)),
                  FileInfo.joinPaths("@loader_path", FileInfo.relativePath("/"+apx.app_bin_path, "/"+apx.app_plugin_path))
              ]
            : [
                  "$ORIGIN",
                  "$ORIGIN/..",
                  "$ORIGIN/../..",
                  FileInfo.joinPaths("$ORIGIN", FileInfo.relativePath("/"+apx.app_plugin_path, "/"+apx.app_library_path))
              ]
}

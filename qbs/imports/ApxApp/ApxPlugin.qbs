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

    appdata.plugins: [
        isForDarwin
        ? bundle.executablePath
        : isForLinux
          ? "lib"+product.targetName+cpp.variantSuffix
          : product.targetName+cpp.variantSuffix
    ]

    targetInstallDir: app.app_plugin_path

    Depends { name: "ApxCore" }
    Depends { name: "ApxGcs" }

    Depends { name: "bundle" }
    bundle.identifierPrefix: app.bundle_identifier+".plugin"

    Properties {
        condition: isForLinux
        cpp.variantSuffix: ".so"
    }

    //COMPILER
    Depends { name: "cpp" }

    //cpp.visibility: "minimal"

    cpp.driverLinkerFlags: base.concat(["-rdynamic"])

    cpp.defines: [
        "PLUGIN_NAME=\""+name+"\"",
    ]

    cpp.rpaths: qbs.targetOS.contains("macos")
            ? [
                  FileInfo.joinPaths("@executable_path", FileInfo.relativePath("/"+app.app_bin_path, "/"+app.app_library_path)),
                 // FileInfo.joinPaths("@loader_path", FileInfo.relativePath("/"+app.app_bin_path, "/"+app.app_plugin_path))
              ]
            : [
                  "$ORIGIN",
                  "$ORIGIN/..",
                  "$ORIGIN/../..",
                  FileInfo.joinPaths("$ORIGIN", FileInfo.relativePath("/"+app.app_plugin_path, "/"+app.app_library_path))
              ]
}

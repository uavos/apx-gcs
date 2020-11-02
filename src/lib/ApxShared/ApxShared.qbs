import qbs.FileInfo
import qbs.File
import ApxApp


ApxApp.ApxLibrary {

    property stringList names: [
        "common",
        "crc",
        "xbus",
        "xbus.uart",
        "xbus.tcp",
        "xbus.telemetry",
        "mandala",
        "mandala.backport",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.cpp.includePaths
    }

    Depends { name: "cpp" }
    Depends { name: "sdk"; submodules: [ "libs", "headers" ] }

    readonly property stringList mnames: names.map(function(s){
        return s.replace(/\./g,"__")
    })
    Depends { name: "apx_libs"; submodules: mnames }

    Group {
        fileTagsFilter: [ "gensrc.output.hpp" ]
        qbs.install: true
        qbs.installDir: FileInfo.joinPaths(app.app_data_path, "scripts/sysroot/include/apx")
        qbs.installSourceBase: FileInfo.joinPaths(product.destinationDirectory, "gensrc/mandala")
    }
}

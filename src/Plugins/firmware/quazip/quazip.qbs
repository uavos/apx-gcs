import qbs
import ApxApp

ApxApp.ApxProduct {

    type: ["staticlibrary"]
    install: false

    Depends {
        name: "Qt";
        submodules: [
            "core",
        ]
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [product.sourceDirectory]
        cpp.dynamicLibraries: ["z"]
    }

    cpp.defines: [
        "QUAZIP_STATIC",
        "QUAZIP_BUILD",
    ]

    cpp.cxxFlags: base
    .concat([
        "-Wno-deprecated-declarations",
    ])


    files: [
        "quazip.cpp", "quazip.h",
        "zip.c", "zip.h",
        "unzip.c", "unzip.h",
        "quazipnewinfo.cpp", "quazipnewinfo.h",
        "quazipfileinfo.cpp", "quazipfileinfo.h",
        "quazipfile.cpp", "quazipfile.h",
        "quazipdir.cpp", "quazipdir.h",
        "quaziodevice.cpp", "quaziodevice.h",
        "quagzipfile.cpp", "quagzipfile.h",
        "quacrc32.cpp", "quacrc32.h",
        "quaadler32.cpp", "quaadler32.h",
        "JlCompress.cpp", "JlCompress.h",
        "qioapi.cpp",
        "quazip_global.h",
        "quachecksum32.h",
        "ioapi.h",
        "minizip_crypt.h",
    ]
}

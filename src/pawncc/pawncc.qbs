import qbs
import apx.Application as APX

APX.ApxProduct {

    bundle.isBundle: false
    targetInstallDir: app.app_bin_path
    Properties {
        condition: qbs.targetOS.contains("linux")
        cpp.dynamicLibraries: "m"
        cpp.defines: outer.concat(["HAVE_ENDIAN_H"])

        qbs.architectures: [ "x86" ]
        multiplexByQbsProperties: ["architectures"]
    }
    Properties {
        condition: qbs.targetOS.contains("macos")
        cpp.defines: outer.concat(["HAVE_STRLCPY","HAVE_STRLCAT"])
    }

    cpp.defines: [
        "PAWN_CELL_SIZE=32",
        "FLOATPOINT",
    ]

    cpp.cFlags: base.concat([
        "-Wno-missing-field-initializers",
        "-Wno-sign-compare",
        "-Wno-unused-parameter",
        "-Wno-logical-op-parentheses",
        "-Wno-shift-negative-value",
        "-Wno-tautological-compare",
        "-Wno-deprecated-declarations",
    ])


    cpp.includePaths: [
        "compiler",
        "linux",
    ]

    Group {
        name: "compiler"
        prefix: name+"/"
        files: [
            "sc.h",
            "sc1.c",
            "sc2.c",
            "sc3.c",
            "sc4.c",
            "sc5.c",
            "sc6.c",
            "sc7.c",
            "sci18n.c",
            "sclist.c",
            "scmemfil.c",
            "scstate.c",
            "scvars.c",
            "lstring.c",
            "memfile.c",
            "scexpand.c",
        ]
    }

    Group {
        name: "linux"
        prefix: name+"/"
        files: [
            "binreloc.c",
            "getch.c",
        ]
    }

    Group {
        name: "amx"
        prefix: name+"/"
        files: [
            "keeloq.c",
        ]
    }
}

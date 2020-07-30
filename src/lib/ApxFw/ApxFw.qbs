import qbs
import qbs.File
import qbs.FileInfo
import ApxApp

Project {

    references: "quazip/quazip.qbs"

    ApxApp.ApxLibrary {

        Depends { name: "ApxCore" }
        Depends { name: "ApxData" }

        Depends { name: "quazip" }

        Depends {
            name: "Qt";
            submodules: [
                "core",
                "network",
            ]
        }


        Depends { name: "cpp" }
        Depends { name: "sdk"; submodules: [ "libs", "headers" ] }

        cpp.includePaths: base.concat([
            sourceDirectory,
        ])

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: product.cpp.includePaths
        }


        files: [
            "ApxFw.cpp", "ApxFw.h",
        ]

    }
}

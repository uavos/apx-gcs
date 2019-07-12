import qbs.FileInfo
import ApxApp

ApxApp.ApxLibrary {


    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.cpp.includePaths
    }

    Depends { name: "cpp" }
    Depends { name: "sdk" }

    Depends {
        name: "ApxShared"
        submodules: [
            "Mandala",
            "Math",
            "Xbus",
        ]
    }

    cpp.defines: ["MANDALA_VMVARS"]

}

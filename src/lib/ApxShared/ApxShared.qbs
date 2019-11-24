import qbs.FileInfo
import ApxApp

ApxApp.ApxLibrary {


    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.cpp.includePaths
    }

    Depends { name: "cpp" }
    Depends { name: "sdk" }

    ModuleProbe {
        id: _modules
        names: [
            "Xbus",
            "Mandala",
            "Math",
            "common",
        ]
        searchPaths: FileInfo.joinPaths(project.sourceDirectory, "../lib")
    }
    Group {
        name: "Modules"
        files: _modules.contents.files
    }


    cpp.includePaths: _modules.searchPaths

    cpp.defines: [
    "MANDALA_VMVARS",
    ]
    .concat(_modules.contents.defines)

}

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
        searchPath: FileInfo.joinPaths(project.sourceDirectory, "../lib")
    }
    Group {
        name: "Modules"
        prefix: _modules.searchPath+"/"
        files: _modules.files
    }


    cpp.includePaths: [
        _modules.searchPath,
    ]

    cpp.defines: [
        "MANDALA_VMVARS",
    ]

}

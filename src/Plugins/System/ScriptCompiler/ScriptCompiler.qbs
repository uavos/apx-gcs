import qbs
import ApxApp
import qbs.FileInfo

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "network",
        ]
    }

    files: [
        "ScriptCompilerPlugin.h",
        "ScriptCompiler.cpp", "ScriptCompiler.h",
    ]

    ApxApp.ApxResource {
        src: "sysroot"
        prefix: FileInfo.joinPaths(project.libDir, "wasm/")
        qbs.installDir: FileInfo.joinPaths(app.app_data_path, "scripts")
        qbs.installSourceBase: prefix
        files: [
            "sysroot/**",
            ".vscode/**",
        ]
        excludeFiles: base.concat(["*.wasm"])
    }

}

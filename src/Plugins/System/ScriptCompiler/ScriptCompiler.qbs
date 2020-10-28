import qbs
import ApxApp

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
        src: "scripts"
        files: [
            "wasm/**",
        ]
    }

}

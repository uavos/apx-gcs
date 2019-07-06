import qbs
import apx.Application as APX

APX.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "gui",
            //"concurrent",
        ]
    }


    files: [
        "SimPlugin.h",
        "Simulator.cpp", "Simulator.h",
    ]

    APX.ApxResource {
        name: "xplane"
        files: [
            "**/*",
        ]
    }
}

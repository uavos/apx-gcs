import qbs
import apx.Application as APX

APX.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "location",
        ]
    }


    files: [
        "MapPlugin.h",
        "MapTools.cpp", "MapTools.h",
    ]

    Depends { name: "qmlqrc" }
    qmlqrc.usePrefix: false
}

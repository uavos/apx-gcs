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

    Depends { name: "ApxData" }
    Depends { name: "qmlqrc" }

    files: [
        "geometrycollector.cpp",
        "geometrycollector.h",
        "kmloverlayplugin.h",
        "kmloverlay.cpp",
        "kmloverlay.h",
    ]

    Properties {
        condition: qbs.targetOS.contains("linux")
        cpp.dynamicLibraries: ["kmldom", "kmlbase", "kmlengine", "kmlconvenience", "kmlregionator", "kmlxsd"]
    }
}

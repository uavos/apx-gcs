import qbs
import apx.Application as APX

APX.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "location",
            "xml"
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
        "kmlpolygonsmodel.cpp",
        "kmlpolygonsmodel.h",
    ]
}

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
        "kmloverlayplugin.h",
        "kmloverlay.cpp",
        "kmloverlay.h",
        "kmlparser.cpp",
        "kmlparser.h",
        "kmlpolygonsmodel.cpp",
        "kmlpolygonsmodel.h",
    ]
}

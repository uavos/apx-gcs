import qbs
import ApxApp

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "location",
            "xml",
            "widgets" //for QFileDialog
        ]
    }

    Depends { name: "ApxData" }
    Depends { name: "qmlqrc" }

    files: [
        "kmlgeopolygon.cpp",
        "kmlgeopolygon.h",
        "kmloverlayplugin.h",
        "kmloverlay.cpp",
        "kmloverlay.h",
        "kmlparser.cpp",
        "kmlparser.h",
        "kmlpolygonsmodel.cpp",
        "kmlpolygonsmodel.h",
    ]
}

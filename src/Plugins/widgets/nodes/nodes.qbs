import qbs
import apx.Application as APX

APX.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "widgets",
            "qml",
            "xml",
        ]
    }

    Depends { name: "ApxData" }

    files: [
        "NodesPlugin.h",
        "NodesFrame.cpp", "NodesFrame.h",
    ]
}

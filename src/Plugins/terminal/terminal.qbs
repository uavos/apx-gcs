import qbs
import apx.Application as APX

APX.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "qml",
        ]
    }


    files: [
        "TerminalPlugin.h",
        "Terminal.cpp", "Terminal.h",
        "TerminalListModel.cpp", "TerminalListModel.h",
    ]

    Depends { name: "qmlqrc" }
}

import qbs
import ApxApp

ApxApp.ApxPlugin {

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
    ]

    Depends { name: "qmlqrc" }
}

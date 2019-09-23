import qbs
import ApxApp

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "widgets",
        ]
    }

    Depends { name: "ApxData" }

    files: [
        "BlackboxPlugin.h",
        "Blackbox.cpp", "Blackbox.h",
        "BlackboxItem.cpp", "BlackboxItem.h",
        "BlackboxNode.cpp", "BlackboxNode.h",
        "BlackboxFile.cpp", "BlackboxFile.h",
        "BlackboxReader.cpp", "BlackboxReader.h",
    ]
}

import qbs
import ApxApp

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "widgets",
        ] }


    files: [
        "JSTreePlugin.h",
        "JSTreeView.cpp", "JSTreeView.h",
    ]
}

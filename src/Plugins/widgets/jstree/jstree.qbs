import qbs
import apx.Application as APX

APX.ApxPlugin {

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

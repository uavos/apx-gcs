import qbs
import apx.Application as APX

APX.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "widgets",
        ] }


    files: [
        "ServosPlugin.h",
        "ServosForm.cpp", "ServosForm.h",
        "ServosForm.ui",
    ]
}

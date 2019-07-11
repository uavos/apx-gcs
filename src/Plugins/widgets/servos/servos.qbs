import qbs
import ApxApp

ApxApp.ApxPlugin {

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

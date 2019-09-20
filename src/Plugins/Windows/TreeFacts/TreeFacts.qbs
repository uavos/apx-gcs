import qbs
import ApxApp

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "widgets",
        ] }


    files: [
        "SystreePlugin.h",
    ]
}

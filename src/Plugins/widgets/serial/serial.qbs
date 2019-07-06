import qbs
import apx.Application as APX

APX.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "widgets",
        ] }

    Depends { name: "ApxData" }

    files: [
        "SerialPlugin.h",
        "SerialForm.cpp", "SerialForm.h",
        "SerialForm.ui",
    ]
}

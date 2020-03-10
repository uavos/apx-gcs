import qbs
import ApxApp

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "widgets",
        ] }


    Depends { name: "ApxData" }

    files: [
        "CompassPlugin.h",
        "CompassFrame.cpp", "CompassFrame.h",
        "DrawingArea.cpp", "DrawingArea.h",
    ]
}

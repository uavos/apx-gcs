import qbs
import apx.Application as APX

APX.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "widgets",
        ] }


    files: [
        "CompassPlugin.h",
        "CompassFrame.cpp", "CompassFrame.h",
        "DrawingArea.cpp", "DrawingArea.h",
    ]
}

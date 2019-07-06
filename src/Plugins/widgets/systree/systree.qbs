import qbs
import apx.Application as APX

APX.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "widgets",
        ] }


    files: [
        "SystreePlugin.h",
    ]
}

import ApxApp

ApxApp.ApxPlugin {

    condition: !qbs.buildVariant.contains("release")

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "widgets",
        ]
    }


    files: [
        "MandalaTreePlugin.h",
        "MandalaTree.cpp", "MandalaTree.h",
        "MandalaTreeFact.cpp", "MandalaTreeFact.h",
    ]


    Depends {
        name: "apx_libs"
        submodules: [
            "Mandala__tree",
            "Mandala__backport",
        ]
    }

}

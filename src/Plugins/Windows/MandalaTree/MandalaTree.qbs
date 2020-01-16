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
    ]


    Depends { name: "ApxData" }

}

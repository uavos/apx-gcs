import qbs
import ApxApp

Project {

    references: [
        "xplane/xplane.qbs",
    ]

    ApxApp.ApxPlugin {

        Depends {
            name: "Qt";
            submodules: [
                "core",
                "gui",
            ]
        }


        files: [
            "SimPlugin.h",
            "Simulator.cpp", "Simulator.h",
        ]

        ApxApp.ApxResource {
            src: "xplane"
            files: [
                "**/*",
            ]
        }
    }
}

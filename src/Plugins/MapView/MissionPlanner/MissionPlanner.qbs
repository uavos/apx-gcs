import qbs
import ApxApp

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "location",
        ]
    }


    files: [
        "MissionPlannerPlugin.h",
        "MissionPlanner.cpp", "MissionPlanner.h",
        "MapPrefs.cpp", "MapPrefs.h",
    ]

    Depends { name: "qmlqrc" }
    qmlqrc.usePrefix: false

    appdata.qtplugins: [ "geoservices" ]

}

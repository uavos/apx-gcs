import qbs
import ApxApp


Project {

    references: "qwt/qwt.qbs"

    ApxApp.ApxPlugin {

        Depends { name: "qwt" }

        Depends {
            name: "Qt";
            submodules: [
                "widgets",
                "qml",
                "sql",
            ]
        }

        files: [
            "TelemetryPlugin.h",
            "TelemetryFrame.cpp", "TelemetryFrame.h",
            "TelemetryPlot.cpp", "TelemetryPlot.h",
        ]
    }
}

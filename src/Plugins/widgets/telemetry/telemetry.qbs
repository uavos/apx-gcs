import qbs
import apx.Application as APX


Project {

    references: "qwt/qwt.qbs"

    APX.ApxPlugin {

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

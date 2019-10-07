import qbs
import ApxApp

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "qml",
        ]
    }


    files: [
        "DatalinkInspectorPlugin.h",
        "DatalinkInspector.cpp", "DatalinkInspector.h",
        "DatalinkInspectorListModel.cpp", "DatalinkInspectorListModel.h",
    ]

    Depends { name: "qmlqrc" }
}

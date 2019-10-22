import qbs
import ApxApp

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "qml",
            "gui",
        ]
    }


    files: [
        "ShortcutsPlugin.h",
        "Shortcuts.cpp", "Shortcuts.h",
        "Shortcut.cpp", "Shortcut.h",
    ]

    Depends { name: "qmlqrc" }

    ApxApp.ApxResource {
        src: "templates"
        files: [
            "shortcuts.json",
        ]
    }

}

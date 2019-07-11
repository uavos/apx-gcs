import qbs
import ApxApp

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "gui",
        ]
    }

    files: [
        "NotificationsPlugin.h",
        "Notifications.cpp", "Notifications.h",
        "NotifyItem.cpp", "NotifyItem.h",
    ]

    Depends { name: "qmlqrc" }
}

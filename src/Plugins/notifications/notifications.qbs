import qbs
import apx.Application as APX

APX.ApxPlugin {

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

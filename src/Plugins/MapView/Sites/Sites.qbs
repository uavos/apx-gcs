import qbs
import ApxApp

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "gui",
            "location",
            "sql"
        ]
    }

    Depends { name: "ApxData" }

    files: [
        "SitesPlugin.h",
        "Sites.cpp", "Sites.h",
        "SiteEdit.cpp", "SiteEdit.h",
        "LookupSites.cpp", "LookupSites.h",
    ]

    Depends { name: "qmlqrc" }
}

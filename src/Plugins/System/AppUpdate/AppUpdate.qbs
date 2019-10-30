import qbs
import ApxApp

Project {

    condition: qbs.targetOS.contains("macos") || qbs.targetOS.contains("linux")

    ApxApp.ApxPlugin {

        Depends {
            name: "Qt";
            submodules: [
                "core",
            ]
        }

        Depends { name: "qmlqrc" }

        files: [
            "UpdaterPlugin.h",
            "Updater.cpp",
            "Updater.h"
        ]

        Properties {
            condition: qbs.targetOS.contains("macos")
            cpp.frameworkPaths: ["/Library/Frameworks"]
            cpp.frameworks: [
                "Sparkle",
                "AppKit",
            ]
            cpp.includePaths: base.concat(["/Library/Frameworks/Sparkle.framework/Headers"])
        }

        Properties {
            condition: qbs.targetOS.contains("linux")
            cpp.staticLibraries: ["/usr/local/lib/libappimageupdate.a",
                "/usr/local/lib/libappimage_shared.a",
                "/usr/local/lib/libcpr.a",
                "/usr/local/lib/libzsync2.a",
                "/usr/local/lib/libzsync.a",
                "/usr/local/lib/librcksum.a",
                "curl"]
        }

        Group {
            condition: qbs.targetOS.contains("macos")
            name: "macos"
            prefix: name+"/"
            files: [
                "SparkleAutoUpdater.mm", "SparkleAutoUpdater.h",
            ]
        }
        Group {
            condition: qbs.targetOS.contains("linux")
            name: "linux"
            files: [
                "appimage/AppImageAutoUpdater.cpp",
                "appimage/AppImageAutoUpdater.h",
            ]
        }

    }
}

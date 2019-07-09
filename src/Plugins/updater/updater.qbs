import qbs
import apx.Application as APX
import apx.Deploy as Deploy

Project {

    condition: qbs.targetOS.contains("macos")

    APX.ApxPlugin {

        Depends {
            name: "Qt";
            submodules: [
                "core",
            ]
        }

        files: [
            "UpdaterPlugin.h",
            "Updater.cpp", "Updater.h",
        ]

        Properties {
            condition: qbs.targetOS.contains("macos")
            cpp.frameworkPaths: ["/Library/Frameworks"]
            cpp.frameworks: [
                "Sparkle",
                "AppKit",
            ]
            cpp.includePaths: base.concat([
                "/Library/Frameworks/Sparkle.framework/Headers",
            ])
        }

        Group {
            name: "sparkle"
            prefix: name+"/"
            files: [
                "SparkleAutoUpdater.mm", "SparkleAutoUpdater.h",
            ]
        }

    }

    Deploy.ApxDeployFramework {
        framework: "Sparkle"
    }
}
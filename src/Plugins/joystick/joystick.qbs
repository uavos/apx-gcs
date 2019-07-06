import qbs
import apx.Application as APX
import apx.Deploy as Deploy

Project {

    APX.ApxPlugin {

        Depends {
            name: "Qt";
            submodules: [
                "core",
                "qml",
                "concurrent"
            ]
        }

        files: [
            "JoystickPlugin.h",
            "Joysticks.cpp", "Joysticks.h",
            "Joystick.cpp", "Joystick.h",
            "JoystickAxis.cpp", "JoystickAxis.h",
        ]

        APX.ApxResource {
            name: "templates"
            files: [
                "joystick.json",
            ]
        }


        Properties {
            condition: qbs.targetOS.contains("macos")
            cpp.frameworkPaths: ["/Library/Frameworks"]
            cpp.frameworks: ["SDL2"]
            cpp.includePaths: base.concat([
                "/Library/Frameworks/SDL2.framework/Headers",
            ])
        }

        Properties {
            condition: qbs.targetOS.contains("linux")
            cpp.includePaths: base.concat([
                "/usr/include/SDL2",
            ])
            cpp.dynamicLibraries: ["SDL2"]
        }

    }

    Deploy.ApxDeployFramework {
        framework: "SDL2"
    }
}

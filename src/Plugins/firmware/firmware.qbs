import qbs
import apx.Application as APX
import apx.Deploy as Deploy

Project {

    references: "quazip/quazip.qbs"

    APX.ApxPlugin {

        Depends { name: "quazip" }

        Depends {
            name: "Qt";
            submodules: [
                "core",
                "network",
                "serialport",
            ]
        }

        Depends { name: "ApxData" }

        files: [
            "FirmwarePlugin.h",
            "Firmware.cpp", "Firmware.h",
            "QueueItem.cpp", "QueueItem.h",
            "Loader.cpp", "Loader.h",
            "Releases.cpp", "Releases.h",
            "FirmwareTools.cpp", "FirmwareTools.h",
            "Initialize.cpp", "Initialize.h",
            "LoaderStm.cpp", "LoaderStm.h",
        ]

        APX.ApxResource {
            name: "firmware"
            files: [
                "**/*",
            ]
        }
    }
}

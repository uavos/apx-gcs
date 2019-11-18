import qbs
import ApxApp

Project {

    references: "quazip/quazip.qbs"

    ApxApp.ApxPlugin {

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
            "FirmwareSelect.cpp", "FirmwareSelect.h",
            "Initialize.cpp", "Initialize.h",
            "LoaderStm.cpp", "LoaderStm.h",
            "Format.cpp", "Format.h",
        ]

        ApxApp.ApxResource {
            src: "firmware"
            files: [
                "**/*",
            ]
        }
    }
}

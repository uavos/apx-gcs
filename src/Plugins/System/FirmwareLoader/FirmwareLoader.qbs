import qbs
import ApxApp


ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "serialport",
        ]
    }

    Depends { name: "ApxData" }
    Depends { name: "ApxFw" }

    files: [
        "FirmwarePlugin.h",
        "Firmware.cpp", "Firmware.h",
        "QueueItem.cpp", "QueueItem.h",
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

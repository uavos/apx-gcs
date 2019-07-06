import qbs
import apx.Application as APX

APX.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "multimedia",
            "texttospeech",
        ]
    }


    files: [
        "SoundsPlugin.h",
        "Sounds.cpp", "Sounds.h",
    ]

    APX.ApxResource {
        name: "audio"
        files: [
            "**/*",
        ]
    }

    APX.ApxResource {
        name: "templates"
        files: [
            "speech.json",
        ]
    }
}

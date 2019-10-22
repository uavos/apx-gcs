import qbs
import ApxApp

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "multimedia",
            "texttospeech",
        ]
    }

    /*appdata.data: {
        var json = base
        json.qtplugins = []
        json.qtplugins.push("texttospeech")
        return json
    }*/
    appdata.qtplugins: [ "texttospeech" ]

    files: [
        "SoundsPlugin.h",
        "Sounds.cpp", "Sounds.h",
    ]

    ApxApp.ApxResource {
        src: "audio"
        files: [
            "**/*",
        ]
    }

    ApxApp.ApxResource {
        src: "templates"
        files: [
            "speech.json",
        ]
    }
}

import qbs
import ApxApp

ApxApp.ApxPlugin {

    Depends {
        name: "Qt";
        submodules: [
            "core",
        ]
    }

    Depends { name: "ApxData" }

    files: [
        "ProtocolBackportPlugin.h",
        "ProtocolBackport.cpp", "ProtocolBackport.h",
        "ProtocolV9.cpp", "ProtocolV9.h",
    ]


    property stringList names: [
        //"Mandala.flat",
    ]

    readonly property stringList mnames: names.map(function(s){
        return s.replace(/\./g,"__")
    })
    Depends { name: "apx_libs"; submodules: mnames }

}

import qbs.FileInfo
import ApxApp

ApxApp.ApxLibrary {

    Depends { name: "ApxCore" }
    Depends { name: "ApxShared" }

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "sql",
            "qml",
            "quick",
            "xml",
            "widgets",
        ]
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.cpp.includePaths
    }

    Depends { name: "cpp" }
    Depends { name: "sdk"; submodules: [ "libs", "headers" ] }

    cpp.includePaths: [
        sourceDirectory,
    ]


    Group {
        name: "Mandala"
        prefix: name+"/"
        files: [
            "Mandala.cpp", "Mandala.h",
            "MandalaFact.cpp", "MandalaFact.h",
        ]
    }

    Group {
        name: "Dictionary"
        prefix: name+"/"
        files: [
            "DictNode.cpp", "DictNode.h",
            "DictMission.cpp", "DictMission.h",
        ]
    }

    Group {
        name: "Protocols"
        prefix: name+"/"
        files: [
            "ProtocolBase.cpp", "ProtocolBase.h",
            "ProtocolViewBase.cpp", "ProtocolViewBase.h",
            "ProtocolStream.h",
            "ProtocolTrace.cpp", "ProtocolTrace.h",
            "ProtocolConverter.cpp", "ProtocolConverter.h",
            "ProtocolVehicles.cpp", "ProtocolVehicles.h",
            "ProtocolVehicle.cpp", "ProtocolVehicle.h",
            "ProtocolNodes.cpp", "ProtocolNodes.h",
            "ProtocolNode.cpp", "ProtocolNode.h",
            "ProtocolNodeRequest.cpp", "ProtocolNodeRequest.h",
            "ProtocolNodeFile.cpp", "ProtocolNodeFile.h",
            "ProtocolTelemetry.cpp", "ProtocolTelemetry.h",

            //"ProtocolMission.cpp", "ProtocolMission.h",
        ]
    }

    Group {
        name: "Database"
        prefix: name+"/"
        files: [
            "Database.cpp", "Database.h",
            "DatabaseSession.cpp", "DatabaseSession.h",
            "DatabaseWorker.cpp", "DatabaseWorker.h",
            "DatabaseRequest.cpp", "DatabaseRequest.h",
            "DatabaseLookup.cpp", "DatabaseLookup.h",
            "DatabaseLookupModel.cpp", "DatabaseLookupModel.h",
            "VehiclesDB.cpp", "VehiclesDB.h",
            "VehiclesStorage.cpp", "VehiclesStorage.h",
            "VehiclesReqDict.cpp", "VehiclesReqDict.h",
            "VehiclesReqNconf.cpp", "VehiclesReqNconf.h",
            "VehiclesReqVehicle.cpp", "VehiclesReqVehicle.h",
            "MissionsDB.cpp", "MissionsDB.h",
            "TelemetryDB.cpp", "TelemetryDB.h",
            "TelemetryReqWrite.cpp", "TelemetryReqWrite.h",
            "TelemetryReqRead.cpp", "TelemetryReqRead.h",
        ]
    }

    Group {
        name: "Sharing"
        prefix: name+"/"
        files: [
            "Share.cpp", "Share.h",
            "ShareXml.cpp", "ShareXml.h",
            //"NodesXml.cpp", "NodesXml.h",
            "MissionsXml.cpp", "MissionsXml.h",
            //"TelemetryXmlImport.cpp", "TelemetryXmlImport.h",
            //"TelemetryXmlExport.cpp", "TelemetryXmlExport.h",
        ]
    }

    ApxApp.ApxResource {
        src: "templates"
        files: [
            "share/*",
        ]
    }

}

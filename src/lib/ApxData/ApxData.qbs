import qbs.FileInfo
import apx.Application as APX

APX.ApxLibrary {

    Depends { name: "ApxCore" }

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "sql",
            "qml",
            "xml",
            "widgets",
        ]
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [
            product.sourceDirectory,
            FileInfo.joinPaths(project.libDir, "include"),
        ]
    }

    Depends { name: "apx" }
    Depends { name: "cpp" }
    Depends { name: "sdk" }

    cpp.includePaths: base
    .concat([
        sourceDirectory,
        project.libDir,
        FileInfo.joinPaths(project.libDir, "include"),
    ])

    Group {
        name: "Lib.Mandala"
        prefix: FileInfo.joinPaths(project.libDir, "Mandala/")
        files: [
            "Mandala.cpp", "Mandala.h",
            "MandalaCore.cpp", "MandalaCore.h",
            "MatrixMath.cpp", "MatrixMath.h",
            "MandalaTemplate.h",
            "MandalaIndexes.h",
            "MandalaConstants.h",
            "preprocessor.h",
        ]
    }
    Group {
        name: "Lib.include"
        prefix: FileInfo.joinPaths(project.libDir, "include/")
        files: [
            "*.h",
        ]
    }
    Group {
        name: "Lib.Xbus"
        prefix: FileInfo.joinPaths(project.libDir, "Xbus/")
        files: [
            "xbus.h",
            "xbus_vehicle.h",
            "xbus_node.h",
            "xbus_node_conf.h",
            "xbus_node_file.h",
            "xbus_node_blackbox.h",
            "escaped.h",
        ]
    }
    Group {
        name: "Lib.other"
        prefix: FileInfo.joinPaths(project.libDir, "other/")
        files: [
            "Mission.h",
        ]
    }
    Group {
        name: "Lib.Math"
        prefix: FileInfo.joinPaths(project.libDir, "Math/")
        files: [
            "crc.c", "crc.h",
        ]
    }

    Group {
        name: "Dictionary"
        prefix: name+"/"
        files: [
            "DictMandala.cpp", "DictMandala.h",
            "DictNode.cpp", "DictNode.h",
            "DictMission.cpp", "DictMission.h",
            "MandalaIndex.h",
        ]
    }

    Group {
        name: "Protocols"
        prefix: name+"/"
        files: [
            "ApxProtocol.cpp", "ApxProtocol.h",
            "ProtocolBase.cpp", "ProtocolBase.h",
            "ProtocolVehicles.cpp", "ProtocolVehicles.h",
            "ProtocolVehicle.cpp", "ProtocolVehicle.h",
            "ProtocolTelemetry.cpp", "ProtocolTelemetry.h",
            "ProtocolMission.cpp", "ProtocolMission.h",
            "ProtocolService.cpp", "ProtocolService.h",
            "ProtocolServiceRequest.cpp", "ProtocolServiceRequest.h",
            "ProtocolServiceNode.cpp", "ProtocolServiceNode.h",
            "ProtocolServiceFile.cpp", "ProtocolServiceFile.h",
            "ProtocolServiceFirmware.cpp", "ProtocolServiceFirmware.h",
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
            "NodesDB.cpp", "NodesDB.h",
            "NodesReqDict.cpp", "NodesReqDict.h",
            "NodesReqNconf.cpp", "NodesReqNconf.h",
            "NodesReqVehicle.cpp", "NodesReqVehicle.h",
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
            "NodesXml.cpp", "NodesXml.h",
            "MissionsXml.cpp", "MissionsXml.h",
            "TelemetryXmlImport.cpp", "TelemetryXmlImport.h",
            "TelemetryXmlExport.cpp", "TelemetryXmlExport.h",
        ]
    }

}

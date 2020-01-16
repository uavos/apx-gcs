import qbs
import qbs.File
import qbs.FileInfo
import ApxApp

ApxApp.ApxLibrary {

    Depends { name: "ApxCore" }
    Depends { name: "ApxData" }
    Depends { name: "ApxShared" }

    Depends {
        name: "Qt";
        submodules: [
            "widgets",
            "xml",
            "quick",
            "quickwidgets",
            "quickcontrols2",
            "svg",
            "sql",
            "network",
            "serialport",
            "multimedia",
            "opengl",
            "location-private",
            "positioning-private",
        ]
    }

    Depends { name: "cpp" }
    Depends { name: "sdk"; submodules: [ "libs", "headers" ] }

    cpp.includePaths: base.concat([
        sourceDirectory,
    ])

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.cpp.includePaths
    }


    Group {
        name: "App"
        prefix: name+"/"
        files: [
            "AppGcs.cpp", "AppGcs.h",
            "AppMenu.cpp", "AppMenu.h",
        ]
    }

    Group {
        name: "TreeModel"
        prefix: name+"/"
        files: [
            "FactDelegate.cpp", "FactDelegate.h",
            "FactDelegateArray.cpp", "FactDelegateArray.h",
            "FactDelegateDialog.cpp", "FactDelegateDialog.h",
            "FactDelegateScript.cpp", "FactDelegateScript.h",
            "FactTreeModel.cpp", "FactTreeModel.h",
            "FactTreeView.cpp", "FactTreeView.h",
            "SourceEdit.cpp", "SourceEdit.h",
            "JSTreeModel.cpp", "JSTreeModel.h",
        ]
    }


    Group {
        name: "Mandala"
        prefix: name+"/"
        condition: !qbs.buildVariant.contains("release")
        files: [
            "MandalaTree.cpp", "MandalaTree.h",
            "MandalaTreeFact.cpp", "MandalaTreeFact.h",
            "MandalaTreeStream.h",
        ]
    }

    Group {
        name: "Vehicles"
        prefix: name+"/"
        files: [
            "Vehicle.cpp", "Vehicle.h",
            "VehicleMandala.cpp", "VehicleMandala.h",
            "VehicleMandalaFact.cpp", "VehicleMandalaFact.h",
            "VehicleMandalaValue.h",
            "VehicleSelect.cpp", "VehicleSelect.h",
            "VehicleWarnings.cpp", "VehicleWarnings.h",
            "Vehicles.cpp", "Vehicles.h",
        ]
    }

    Group {
        name: "Telemetry"
        prefix: name+"/"
        files: [
            "Telemetry.cpp", "Telemetry.h",
            "TelemetryRecorder.cpp", "TelemetryRecorder.h",
            "LookupTelemetry.cpp", "LookupTelemetry.h",
            "TelemetryReader.cpp", "TelemetryReader.h",
            "TelemetryReaderDataReq.cpp", "TelemetryReaderDataReq.h",
            "TelemetryPlayer.cpp", "TelemetryPlayer.h",
            "TelemetryShare.cpp", "TelemetryShare.h",
        ]
    }

    Group {
        name: "Nodes"
        prefix: name+"/"
        files: [
            "NodesBase.cpp", "NodesBase.h",
            "NodeField.cpp", "NodeField.h",
            "NodeItem.cpp", "NodeItem.h",
            "NodeItemBase.cpp", "NodeItemBase.h",
            "NodeItemData.cpp", "NodeItemData.h",
            "NodesStorage.cpp", "NodesStorage.h",
            "NodeTools.cpp", "NodeTools.h",
            "NodeToolsGroup.cpp", "NodeToolsGroup.h",
            "LookupNodeBackup.cpp", "LookupNodeBackup.h",
            "LookupConfigs.cpp", "LookupConfigs.h",
            "Nodes.cpp", "Nodes.h",
            "NodesShare.cpp", "NodesShare.h",
        ]
    }

    Group {
        name: "Mission"
        prefix: name+"/"
        files: [
            "MissionField.cpp", "MissionField.h",
            "MissionGroup.cpp", "MissionGroup.h",
            "MissionItem.cpp", "MissionItem.h",
            "MissionListModel.cpp", "MissionListModel.h",
            "MissionMapItemsModel.cpp", "MissionMapItemsModel.h",
            "MissionStorage.cpp", "MissionStorage.h",
            "LookupMissions.cpp", "LookupMissions.h",
            "MissionTools.cpp", "MissionTools.h",
            "MissionShare.cpp", "MissionShare.h",
            "Poi.cpp", "Poi.h",
            "Runway.cpp", "Runway.h",
            "Taxiway.cpp", "Taxiway.h",
            "Area.cpp", "Area.h",
            "VehicleMission.cpp", "VehicleMission.h",
            "Waypoint.cpp", "Waypoint.h",
            "WaypointActions.cpp", "WaypointActions.h",
        ]
    }

    Group {
        name: "Datalink"
        prefix: name+"/"
        files: [
            "Datalink.cpp", "Datalink.h",
            "DatalinkConnection.cpp", "DatalinkConnection.h",
            "DatalinkServer.cpp", "DatalinkServer.h",
            "DatalinkRemotes.cpp", "DatalinkRemotes.h",
            "DatalinkRemote.cpp", "DatalinkRemote.h",
            "DatalinkPorts.cpp", "DatalinkPorts.h",
            "DatalinkPort.cpp", "DatalinkPort.h",
            "DatalinkTcpSocket.cpp", "DatalinkTcpSocket.h",
            "DatalinkSerial.cpp", "DatalinkSerial.h",
            "DatalinkStats.cpp", "DatalinkStats.h",
            "HttpService.cpp", "HttpService.h",
        ]
    }

    Group {
        name: "Pawn"
        prefix: name+"/"
        files: [
            "PawnCompiler.cpp", "PawnCompiler.h",
        ]
    }

    ApxApp.ApxResource {
        src: "scripts"
        files: [
            "pawn/**/*",
        ]
    }

}

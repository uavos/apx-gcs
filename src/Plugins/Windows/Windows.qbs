import qbs.FileInfo

Project {

    property var plugins: [
        "CompassCalibration",
        "SerialPortConsole",
        "ServoConfig",
        "TelemetryChart",
        "TreeFacts",
        "TreeJS",
        "VehicleConfiguration",
        "MandalaTree",
    ]

    references: {
        var refList=[]
        for(var i in plugins){
            var n=plugins[i]
            refList.push(FileInfo.joinPaths(n,n+".qbs"))
        }
        return refList
    }
}

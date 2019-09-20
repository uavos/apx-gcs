import qbs.FileInfo

Project {

    property var plugins: [
        "AppUpdate",
        //"DatalinkInspector",
        "FirmwareLoader",
        "Joystick",
        "Notifications",
        "Simulator",
        "Sounds",
        "Terminal",
        "VideoStreaming",
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

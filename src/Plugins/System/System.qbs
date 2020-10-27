import qbs.FileInfo

Project {

    property var plugins: [
        "AppUpdate",
        "FirmwareLoader",
        "Joystick",
        //"Notifications",
        "Simulator",
        "Sounds",
        "Terminal",
        //"Blackbox",
        "Shortcuts",
        //"ProtocolBackport",
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

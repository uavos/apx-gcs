import qbs
import qbs.FileInfo

Project {

    property var plugins: [
        "sounds",
        "terminal",
        "streaming",
        "location",
        "sim",
        "sites",
        "map",
        "joystick",
        "updater",
        "firmware",
        "notifications",
    ]

    property var widgets: [
        "telemetry",
        "nodes",
        "systree",
        "jstree",
        "compass",
        "serial",
        "servos",
    ]

    references: {
        var refList=[]
        for(var i=0;i<plugins.length;++i){
            var n=plugins[i]
            refList.push(FileInfo.joinPaths(n,n+".qbs"))
        }
        for(var i=0;i<widgets.length;++i){
            var n=widgets[i]
            refList.push(FileInfo.joinPaths("widgets",n,n+".qbs"))
        }
        return refList
    }
}

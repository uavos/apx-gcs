
import qbs.FileInfo

Project {
    qbsSearchPaths: [
        FileInfo.joinPaths(sourceDirectory, "qbs"),
        FileInfo.joinPaths(sourceDirectory, "../lib/qbs"),
    ]

    property path resorcesDir: FileInfo.joinPaths(sourceDirectory, "resources")

    property string arch: "x86_64"

    references: [
        "src/lib/lib.qbs",
        "src/main/main.qbs",
        "src/Plugins/plugins.qbs",
        "src/pawncc/pawncc.qbs",

        //"../nodes/src/sim/sim.qbs",

        "deploy/deploy.qbs",
    ]

    Product {
        name: "Qbs files ("+project.name+")"
        Group {
            name: "qbs"
            files: {
                var v = []
                for(var i in project.qbsSearchPaths){
                    var p = project.qbsSearchPaths[i]
                    p=FileInfo.cleanPath(FileInfo.joinPaths(p, "/**/*"))
                    v.push(p)
                }
                return v
            }
        }
    }
}

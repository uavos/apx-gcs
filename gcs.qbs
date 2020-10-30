
import qbs.FileInfo

Project {
    qbsSearchPaths: [
        FileInfo.joinPaths(sourceDirectory, "qbs"),
        FileInfo.joinPaths(sourceDirectory, "../lib/qbs"),
    ]

    property path resorcesDir: FileInfo.joinPaths(sourceDirectory, "resources")
    property path libDir: FileInfo.joinPaths(sourceDirectory, "../lib")

    property string target_arch: "x86_64"

    references: [
        "src/lib/lib.qbs",
        "src/main/main.qbs",
        "src/Plugins/Plugins.qbs",

        //"deploy/deploy.qbs",
        //"translations/translations.qbs",
    ]

    Product {
        name: "QBS ("+project.name+")"
        Group {
            name: "qbs"
            files: {
                var v = []
                for(var i in project.qbsSearchPaths){
                    var p = project.qbsSearchPaths[i]
                    p=FileInfo.cleanPath(FileInfo.joinPaths(p, "/**/*"))
                    v.push(p)
                }
                v.push(FileInfo.cleanPath(FileInfo.joinPaths(sourceDirectory, "../lib/**/*.qbs")))
                return v
            }
        }
    }
}

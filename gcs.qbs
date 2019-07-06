
import qbs.FileInfo

Project {
    qbsSearchPaths: [
      FileInfo.joinPaths(sourceDirectory, "qbs"),
    ]

    property path resorcesDir: FileInfo.joinPaths(sourceDirectory, "resources")
    property path libDir: FileInfo.joinPaths(sourceDirectory, "../lib")

    property string arch: "x86_64"

    references: [
        "src/lib/lib.qbs",
        "src/main/main.qbs",
        "src/Plugins/plugins.qbs",
        "src/pawncc/pawncc.qbs",
        //"src/sim/sim.qbs",

        "deploy/deploy.qbs",
    ]

    Product {
        name: "Qbs files ("+project.name+")"
        Group {
            name: "qbs"
            prefix: "qbs/**/"
            files: ["*"]
        }
    }
}

import qbs
import qbs.Environment
import qbs.File
import qbs.FileInfo
import qbs.Process

Module {
    property bool usePrefix: true
    property string sourcePath: "qml"

    Depends { name: "Qt.core" }

    Group {
        name: "QML"
        prefix: FileInfo.joinPaths(product.sourceDirectory,qmlqrc.sourcePath)+"/"
        files: [
            "**/*.qml",
            "**/qmldir",
            "**/*.svg",
            "**/*.js",
            "**/*.frag",
            "**/*.fsh",
            "**/qmldir",
            "**/*.conf",
        ]
        fileTags: "qt.core.resource_data"
        Qt.core.resourcePrefix: qmlqrc.usePrefix?product.name+"/":"/"
        Qt.core.resourceSourceBase: qmlqrc.sourcePath+"/"
    }
    Qt.core.resourceFileBaseName: product.name
}

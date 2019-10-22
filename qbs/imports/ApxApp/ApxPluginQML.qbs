import qbs
import qbs.FileInfo
import qbs.File

ApxPlugin {

    //type: "qrc"
    //targetInstallDir: app.app_plugin_path

    Depends { name: "qmlqrc" }
    qmlqrc.usePrefix: true
    qmlqrc.sourcePath: "."



    /*Group {
        fileTagsFilter: product.type
        qbs.install: product.install
        qbs.installDir: product.targetInstallDir
        qbs.installSourceBase: product.destinationDirectory
    }*/


}

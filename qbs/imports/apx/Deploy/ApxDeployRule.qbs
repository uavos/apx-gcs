import qbs
import qbs.FileInfo

Rule{
    multiplex: true
    alwaysRun: true
    inputsFromDependencies: ["installable"]

    outputFileTags: [ "deployment" ]
    outputArtifacts: {
        var v=[]
        for(var i in product.outputs){
            v.push({
                       filePath: FileInfo.joinPaths(product.moduleProperty("qbs","installRoot"), product.moduleProperty("app","app_bundle_path"), product.outputs[i]),
                       fileTags: [ "deployment" ]
                   })
        }
        return v
    }
}

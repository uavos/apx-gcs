import qbs
import qbs.File
import qbs.FileInfo
import qbs.TextFile

Module {


    additionalProductTypes: ["appdata.deploy"]

    property var data: ({})
    property var plugins: []
    property var qtplugins: []

    Rule {
        inputs: product.type.filter(function(v){return v!="appdata.deploy"}) //["dynamiclibrary"]
        multiplex: true
        Artifact {
            filePath: product.name+"-appdata.json"
            fileTags: ["appdata.deploy"]
            qbs.install: false
        }

        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "preparing appdata: " + product.name
            cmd.sourceCode = function() {
                var json = product.appdata.data
                if(product.appdata.plugins.length>0)
                    json.plugins=product.appdata.plugins
                if(product.appdata.qtplugins.length>0)
                    json.qtplugins=product.appdata.qtplugins
                //frameworks
                json.frameworks=[]
                var f=product.cpp.frameworks
                var paths=product.cpp.frameworkPaths
                .concat(product.cpp.compilerFrameworkPaths)
                .concat(product.cpp.systemFrameworkPaths)
                .concat(product.cpp.distributionFrameworkPaths)
                for(var i in f){
                    var fname=f[i]+".framework"
                    var v=fname
                    for(var p in paths){
                        p=FileInfo.joinPaths(paths[p],fname)
                        if(!File.exists(p)) continue
                        v=p
                        break
                    }
                    if(!v)continue
                    v=FileInfo.cleanPath(v)
                    if(json.frameworks.contains(v)) continue
                    json.frameworks.push(v)
                }
                if(json.frameworks.length<=0)
                    delete json.frameworks
                //dynamic libs
                json.libs=[]
                var f=product.cpp.dynamicLibraries
                var paths=product.cpp.libraryPaths
                .concat(product.cpp.compilerLibraryPaths)
                .concat(product.cpp.systemLibraryPaths)
                .concat(product.cpp.distributionLibraryPaths)
                .concat(product.cpp.systemRunPaths)
                for(var i in f){
                    var fname="lib"+f[i]
                    if(product.qbs.targetOS.contains("macos"))fname=fname+".dylib"
                    else if(product.qbs.targetOS.contains("linux"))fname=fname+".so"
                    else if(product.qbs.targetOS.contains("windows"))fname=fname+".dll"
                    var v=fname
                    for(var p in paths){
                        p=FileInfo.joinPaths(paths[p],fname)
                        if(!File.exists(p)) continue
                        v=p
                        break
                    }
                    if(!v)continue
                    v=FileInfo.cleanPath(v)
                    if(json.libs.contains(v)) continue
                    json.libs.push(v)
                }
                if(json.libs.length<=0)
                    delete json.libs

                if(product.type.contains("application")){
                    json.executables = []
                    json.executables.push(product.targetName)
                    /*if(product.bundle.isBundle)
                        json.executables.push(product.bundle.executablePath)
                    else
                        json.executables.push(FileInfo.joinPaths(product.app.app_bin_path, product.targetName))
                        */
                }

                //write metadata to file
                var file = new TextFile(output.filePath, TextFile.WriteOnly);
                file.write(JSON.stringify(json,0,2))
                file.close()
            }
            return [cmd];
        }
    }
}

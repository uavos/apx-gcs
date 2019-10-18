import qbs
import qbs.File
import qbs.FileInfo
import qbs.TextFile

Module {


    additionalProductTypes: ["appdata.deploy"]

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
                var json={}
                //frameworks
                json.frameworks=[]
                var f=product.cpp.frameworks
                var paths=product.cpp.frameworkPaths
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
                    if(json.frameworks.contains(v)) continue
                    json.frameworks.push(v)
                }
                if(json.frameworks.length<=0)
                    delete json.frameworks
                //dynamic libs
                json.libs=[]
                var f=product.cpp.dynamicLibraries
                var paths=product.cpp.libraryPaths
                for(var i in f){
                    var fname="lib"+f[i]+".so"
                    var v=fname
                    for(var p in paths){
                        p=FileInfo.joinPaths(paths[p],fname)
                        if(!File.exists(p)) continue
                        v=p
                        break
                    }
                    if(!v)continue
                    if(json.libs.contains(v)) continue
                    json.libs.push(v)
                }
                if(json.libs.length<=0)
                    delete json.libs
                //write metadata to file
                var file = new TextFile(output.filePath, TextFile.WriteOnly);
                file.write(JSON.stringify(json,0,2))
                file.close()
            }
            return [cmd];
        }
    }
}

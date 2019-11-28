import qbs.FileInfo
import qbs.TextFile

Product {
    name: "MetaData"

    type: ["appdata.deploy.all"]

    Depends { name: "app" }
    Depends { name: "Qt.core" }

    Depends {
        productTypes: [
            "loadablemodule",
            "dynamiclibrary",
            "application",
        ]
    }

    Rule {
        inputsFromDependencies: ["appdata.deploy"]
        inputs: ["appdata.deploy"]
        multiplex: true

        Artifact {
            fileTags: ["appdata.deploy.all"]
            filePath: "appdata.json"
            qbs.install: true
            qbs.installPrefix: ""
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.highlight = "filegen"
            cmd.description = "creating appdata: " + output.filePath
            cmd.sourceCode = function() {
                var appdata={}
                //collect appdata from products
                var tag=inputs["appdata.deploy"]
                for(var i in tag){
                    var inp=tag[i]
                    var jfile = new TextFile(inp.filePath, TextFile.ReadOnly);
                    var json=JSON.parse(jfile.readAll())
                    jfile.close()
                    //join json objects
                    for(var j in json) {
                            if(appdata.hasOwnProperty(j)){
                                for(var k in json[j]){
                                    if(appdata[j].contains(json[j][k]))
                                        continue
                                    appdata[j].push(json[j][k])
                                    appdata[j].sort()
                                }
                            }else{
                                if(json[j].constructor === Array)
                                    appdata[j]=json[j]
                                else
                                    appdata[j]=json[j]
                            }
                    }
                }
                //app metadata section
                appdata.app = {} //product.app
                appdata.app.arch = project.target_arch
                appdata.app.version = product.app.version
                appdata.app.name = product.app.app_display_name
                appdata.app.app_id = product.app.app_id
                appdata.app.platform = product.qbs.targetPlatform

                appdata.app.packages_path = product.app.packages_path
                appdata.app.bundle_path = product.app.app_bundle_path
                appdata.app.bin_path = product.app.app_bin_path
                appdata.app.library_path = product.app.app_library_path
                appdata.app.plugin_path = product.app.app_plugin_path
                appdata.app.data_path = product.app.app_data_path

                //deploy source data
                appdata.app.qt_bin = product.Qt.core.binPath
                appdata.app.src = FileInfo.joinPaths(project.sourceDirectory, "src")

                var file = new TextFile(output.filePath, TextFile.WriteOnly);
                file.write(JSON.stringify(appdata,0,2))
                file.close()
            }
            return [cmd];
        }
    }
}

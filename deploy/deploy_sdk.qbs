import qbs
import qbs.File
import qbs.FileInfo
import qbs.TextFile

Product {

    name: "SDK"

    //libraries, should be placed in sdk:
    Depends { name: "ApxCore" }
    Depends { name: "ApxData" }
    Depends { name: "ApxGcs" }
    Depends { name: "ApxShared" }


    Group {
        name: "Extra SDK files"
        fileTags: ["sdk.extras"]
        prefix: FileInfo.joinPaths(project.sourceDirectory, "/")
        files: [
            "src/main/qml/Apx/Application/GroundControl.qml",
            "src/main/qml/Apx/Application/InstrumentsLayout.qml",
            "src/main/qml/Apx/Application/MainLayout.qml",
            "src/main/qml/Apx/Application/MainPluginFrame.qml",
        ]
    }


    type: ["archiver.archive"]
    Depends { name: "archiver" }
    Depends { name: "app" }
    Depends { name: "git" }

    Group {
        fileTagsFilter: ["archiver.archive"]
        qbs.install: true
        qbs.installDir: app.packages_path
    }

    Rule {
        inputsFromDependencies: ["sdk.prepare"]
        inputs: ["sdk.extras"]
        multiplex: false
        Artifact {
            fileTags: ["sdk"]
            qbs.install: false
            filePath: {
                if(input.fileTags.contains("sdk.extras"))
                {
                    return FileInfo.joinPaths("sdk/extras",input.fileName)
                }
                else if(input.filePath.indexOf(input.product.destinationDirectory) == 0)
                {
                    var tail = input.filePath.replace(input.product.destinationDirectory + "/", '')
                    return tail
                }
                else
                {
                    throw("Can't define sdk path for file" + input.fileName)
                    return ""
                }
            }
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.highlight = "filegen"
            cmd.description = "adding to sdk package " + input.fileName
            cmd.sourceCode = function() {
                File.copy(input.filePath, output.filePath)
            }
            return [cmd];
        }
    }
    Rule {
        inputs: ["sdk"]
        multiplex: true
        Artifact {
            fileTags: ["archiver.input-list"]
            qbs.install: false
            filePath: "sdk_files_list.txt"
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "prepare file list for sdk package"
            cmd.sourceCode = function() {
                var file = new TextFile(output.filePath, TextFile.WriteOnly);
                for(var i = 0; i < inputs.sdk.length; i++)
                {
                    var path = inputs.sdk[i].filePath.replace(product.destinationDirectory + "/sdk/", '')
                    file.writeLine(path)
                }
                file.close()
            }
            return [cmd];
        }
    }
    archiver.type: "tar"
    archiver.workingDirectory: destinationDirectory + "/sdk"
    archiver.archiveBaseName: "APX_SDK-"+git.probe.version+"-"+qbs.targetPlatform+"-"+project.target_arch
}

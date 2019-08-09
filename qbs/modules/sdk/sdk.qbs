import qbs
import qbs.File
import qbs.FileInfo

Module {

    additionalProductTypes: ["sdk.prepare"]

    Rule {
        inputs: ["dynamiclibrary", "dynamiclibrary_symlink"]
        multiplex: false
        Artifact {
            filePath: {
                var targetFileName = input.fileName;
                if(input.qbs.targetOS.contains("macos"))
                    targetFileName = "lib" + targetFileName + ".dylib"
                return "sdk/lib/" + targetFileName
            }
            fileTags: ["sdk.prepare"]
            qbs.install: false
        }

        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "preparing for sdk " + input.fileName
            cmd.sourceCode = function(inputs, outputs) {
                File.copy(input.filePath, output.filePath)
            }
            return [cmd];
        }
    }

    Rule {
        inputs: ["hpp"]
        multiplex: false
        Artifact {
            filePath: {
                var dest = "sdk/include"
                var inp = FileInfo.cleanPath(input.filePath)
                var rpath =
                        [
                            FileInfo.cleanPath(product.sourceDirectory),
                            FileInfo.cleanPath(FileInfo.joinPaths(project.sourceDirectory, "../lib")),
                        ]
                for(var i in rpath){
                    if(inp.indexOf(rpath[i]) == 0)
                    {
                        var tail = FileInfo.relativePath(rpath[i], inp)
                        return FileInfo.cleanPath(FileInfo.joinPaths(dest, product.name, tail))
                    }
                }
                //console.info(input.filePath)
                return FileInfo.joinPaths(dest, product.name, input.fileName)
            }
            fileTags: ["sdk.prepare"]
            qbs.install: false
        }

        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.highlight = "filegen"
            cmd.description = "preparing for sdk " + input.fileName
            cmd.sourceCode = function() {
                File.copy(input.filePath, output.filePath)
            }
            return [cmd];
        }
    }
}

import qbs
import qbs.File

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
                var start = input.filePath.indexOf(product.sourceDirectory);
                var path = "sdk/include"
                if(start == 0)
                {
                    var tail = input.filePath.replace(product.sourceDirectory, '')
                    path += tail
                }
                else
                {
                    path += "/" + product.name + "/" + input.fileName
                }
                return path
            }
            fileTags: ["sdk.prepare"]
            qbs.install: false
            property string hello: "hello world"
        }

        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "preparing for sdk " + input.fileName
            cmd.sourceCode = function() {
                File.copy(input.filePath, output.filePath)
            }
            return [cmd];
        }
    }
}

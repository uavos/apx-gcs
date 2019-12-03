import qbs.FileInfo
import qbs.File
import ApxApp


ApxApp.ApxLibrary {

    property stringList names: [
        "Xbus",
        "Xbus.uart",
        "Mandala",
        "crc",
        "common",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.cpp.includePaths
    }

    Depends { name: "cpp" }
    Depends { name: "sdk"; submodules: [ "libs", "headers" ] }

    readonly property stringList mnames: names.map(function(s){
        return s.replace(/\./g,"__")
    })
    Depends { name: "apx_libs"; submodules: mnames }
    /*Rule {
        inputsFromDependencies: ["sdk.headers"]
        multiplex: false
        Artifact {
            filePath: {
                var dest = "sdk/include"
                var inp = FileInfo.cleanPath(input.filePath)
                var tail = inp.slice(inp.indexOf(dest)+dest.length)
                tail = tail.replace(project.name+".libs.", "")
                return FileInfo.joinPaths(dest, product.name, tail)
            }
            fileTags: ["sdk.prepare"]
            qbs.install: false
        }

        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.highlight = "filegen"
            cmd.description = "preparing for sdk " + input.fileName
            cmd.sourceCode = function() {
                console.info(output.filePath)
                File.copy(input.filePath, output.filePath)
            }
            return [cmd];
        }
    }*/

}

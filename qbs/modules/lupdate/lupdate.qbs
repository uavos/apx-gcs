import qbs
import qbs.File
import qbs.FileInfo

Module {
    condition: true
    property string tsFileName: product.targetName
    property string tsFilesDir: product.destinationDirectory
    // FileTagger {
    //     pattern: "*.cpp"
    //     fileTags: ["lupdate.input"]
    // }
    Rule {
        id: translator
        multiplex: true
        inputs: ["lupdate.input"]
        Artifact {
            fileTags: ["lupdate.ts"]
            filePath: FileInfo.joinPaths(product.lupdate.tsFilesDir,
                                         product.lupdate.tsFileName+".fake.ts")
        }
        
        prepare: {
            var qt_bin_path = product.moduleProperty("Qt.core", "binPath")
            var targetOS = product.moduleProperty("qbs", "targetOS")
            if (targetOS.contains("macos")) {
                lupdate = FileInfo.joinPaths(qt_bin_path, "lupdate")
            } else if (targetOS.contains("linux")) {
                lupdate = FileInfo.joinPaths(qt_bin_path, "lupdate")
            } else if (targetOS.contains("windows")) {
                lupdate = FileInfo.joinPaths(qt_bin_path, "lupdate.exe")
            }

            console.info(inputs)
            for(var tag in inputs){
                for(var i in inputs[tag]){
                    var inp=tag[i]
                    console.info(inp)
                    //args.push(inp.filePath)
                }
            }

            var out_ts = FileInfo.joinPaths(project.buildDirectory, "app.ts")
            args.push("-ts")
            args.push(out_ts)

            // some obvious code skipped. filling args with cpp inputs, '-ts' and output,file
            var cmd = new Command(lupdate, args)
            var cmdCopy = new JavaScriptCommand();
            cmdCopy.sourceCode = function() { File.copy(out_ts, output.filePath); }
            return [cmd, cmdCopy];
        }
    }
}

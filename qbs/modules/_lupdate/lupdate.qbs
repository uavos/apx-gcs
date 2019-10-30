import qbs
import qbs.File
import qbs.FileInfo

Module {
    condition: true
    property string tsFileName: product.targetName
    property string tsFilesDir: product.destinationDirectory
    Rule {
        multiplex: true
        inputs: ["lupdate.input"]
        inputsFromDependencies: ["cpp", "qrc"]
        Artifact {
            fileTags: ["lupdate.ts"]
            filePath: FileInfo.joinPaths(product.lupdate.tsFilesDir,
                                         product.lupdate.tsFileName+".ts")
        }
        
        prepare: {
            var cmds = []
            var qt_bin_path = product.moduleProperty("Qt.core", "binPath")
            var targetOS = product.moduleProperty("qbs", "targetOS")
            if (targetOS.contains("macos")) {
                lupdate = FileInfo.joinPaths(qt_bin_path, "lupdate")
            } else if (targetOS.contains("linux")) {
                lupdate = FileInfo.joinPaths(qt_bin_path, "lupdate")
            } else if (targetOS.contains("windows")) {
                lupdate = FileInfo.joinPaths(qt_bin_path, "lupdate.exe")
            }

            var args = []

            //console.info(inputs)
            for(var itag in inputs){
                var tag=inputs[itag]
                for(var i in tag){
                    var inp=tag[i]
                    if(inp.filePath.endsWith("qrc"))
                        console.info(inp.filePath)
                    args.push(inp.filePath)
                }
            }

            var out_ts = FileInfo.joinPaths(project.buildDirectory, "app.ts")
            args.push("-silent")
            args.push("-ts")
            args.push(out_ts)

            // some obvious code skipped. filling args with cpp inputs, '-ts' and output,file
            var cmd = new Command(lupdate, args)
            cmd.highlight = "filegen"
            cmd.description = "updating translations: " + output.filePath
            cmds.push(cmd)

            cmd = new Command('touch', output.filePath)
            cmd.silent = true
            cmds.push(cmd)

            return cmds
        }
    }
}

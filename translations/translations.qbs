import qbs.FileInfo

Project {

    condition: qbs.buildVariant.contains("release")



    Product {
        type: ["lupdate.ts", "qm"]


        Group {
            name: "ts"
            files: [ "*.ts" ]
            fileTags: "ts"
        }

        //INSTALL qm
        Group {
            fileTagsFilter: ["qm"]
            qbs.install: true
            qbs.installSourceBase: product.destinationDirectory
            qbs.installDir: FileInfo.joinPaths(product.app.app_data_path, "translations")
        }


        //create default.ts source
        Depends { name: "Qt.core" }
        Depends { name: "app" }


        Rule {
            multiplex: true
            requiresInputs: false
            Artifact {
                fileTags: ["lupdate.ts"]
                filePath: FileInfo.joinPaths(product.destinationDirectory,"default.ts")
                qbs.install: true
                qbs.installSourceBase: product.destinationDirectory
                qbs.installDir: FileInfo.joinPaths(product.app.app_data_path, "translations")
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

                args.push(project.sourceDirectory)

                args.push("-locations")
                args.push("none")

                args.push("-source-language")
                args.push("en_US")

                args.push("-ts")
                args.push(output.filePath)

                var cmd = new Command(lupdate, args)
                cmd.highlight = "filegen"
                cmd.description = "updating translations: " + output.filePath
                cmds.push(cmd)

                return cmds
            }
        }
    }
}

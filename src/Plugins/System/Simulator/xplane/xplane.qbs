import qbs.File
import qbs.FileInfo
import ApxApp


Project {
    id: _xplane_plugin
    name: "xplane_plugin"

    qbs.architectures: [ "x86", "x86_64" ]

    ApxSharedLibs {
        name: project.name+".libs"
        names: [
            "Xbus",
            "MandalaCore",
            "Mandala",
            "TcpLink",
            "Calc",
            "common",
        ]
        qbs.architectures: _xplane_plugin.qbs.architectures
    }

    ApxApp.ApxProduct {

        type: "xplane_plugin"

        targetName: "ApxSIL"
        targetInstallDir: app.app_data_path+"/xplane"

        bundle.isBundle: false

        qbs.architectures: project.qbs.architectures //[ "x86", "x86_64" ]
        multiplexByQbsProperties: ["architectures"]
        aggregate: false

        Depends { name: "version_hpp" }

        property path libPath: FileInfo.joinPaths(project.sourceDirectory, "../lib")

        /*ModuleProbe {
            id: _modules
            names: [
                "Xbus",
                "Mandala",
                "TcpLink",
                "Math",
            ]
            searchPaths: FileInfo.joinPaths(project.sourceDirectory, "../lib")
        }
        Group {
            name: "Modules"
            files: _modules.contents.files
        }*/
        Depends {
            productTypes: ["staticlibrary"]
            limitToSubProject: true
        }


        Rule {
            inputs: "dynamiclibrary"
            Artifact {
                filePath: product.targetName+"_"+product.moduleProperty("qbs","architecture")+".xpl"
                fileTags: ["xplane_plugin"]
            }
            prepare: {
                var cmd = new JavaScriptCommand();
                cmd.description = input.fileName + "->" + output.fileName;
                cmd.highlight = "codegen";
                cmd.sourceCode = function() {
                    File.copy(input.filePath,output.filePath)
                }
                return [cmd];
            }
        }


        Properties {
            condition: qbs.targetOS.contains("macos")
            cpp.defines: outer.concat([ "APL=1", "IBM=0", "LIN=0" ])
            //cpp.frameworkPaths: ["SDK/Libraries/Mac"]
            //cpp.frameworks: [ "XPLM", "XPWidgets"]
            //cpp.allowUnresolvedSymbols: true
            cpp.linkerFlags: base.concat([
                                             "-flat_namespace",
                                             "-undefined", "suppress",
                                         ])
        }

        Properties {
            condition: qbs.targetOS.contains("linux")
            cpp.defines: outer.concat([ "APL=0", "IBM=0", "LIN=1" ])
            //cpp.libraryPaths: ["SDK/Libraries/Win"]
            //cpp.staticlibraries: [ "XPLM", "XPWidgets"]
        }

        Properties {
            condition: qbs.targetOS.contains("windows")
            cpp.defines: outer.concat([ "APL=0", "IBM=1", "LIN=0" ])
            //cpp.libraryPaths: ["SDK/Libraries/Win"]
            //cpp.staticlibraries: [ "XPLM", "XPWidgets"]
        }

        cpp.defines: base
        .concat([
        "XPLM200",
        ])

        cpp.includePaths: [libPath]
        .concat([
        "SDK/CHeaders/XPLM",
        ])

        cpp.cxxFlags: base
        .concat([
        "-Wno-unused-parameter",
        "-Wno-unused-variable",
        ])
        cpp.cFlags: base
        .concat([
        "-Wno-unused-parameter",
        ])



        files: [
            "plugin.cpp",
        ]


        Group {
            name: "SDK"
            files: [
                "SDK/CHeaders/XPLM/*.h",
            ]
        }
    }
}

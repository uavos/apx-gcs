import qbs.File
import qbs.FileInfo
import ApxApp

ApxApp.ApxProduct {

    type: "xplane_plugin"

    targetName: "ApxSIL"
    targetInstallDir: app.app_data_path+"/xplane"

    bundle.isBundle: false

    qbs.architectures: project.qbs.architectures //[ "x86", "x86_64" ]
    multiplexByQbsProperties: ["architectures"]
    aggregate: false

    Depends { name: "version_hpp" }

    Depends {
        name: "apx_libs"
        submodules: [
            "Xbus",
            "Xbus__tcp",
            "Mandala",
        ]
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

    cpp.includePaths: base
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


import qbs
import qbs.File
import apx.Application as APX

APX.ApxProduct {

    type: "xplane_plugin"

    targetName: "ApxSIL"
    targetInstallDir: apx.app_data_path+"/xplane"

    bundle.isBundle: false

    qbs.architectures: [ "x86", "x86_64" ]
    multiplexByQbsProperties: ["architectures"]
    aggregate: false

    Depends { name: "apx_version" }

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

    cpp.defines: base.concat([
        "XPLM200",
    ])

    cpp.includePaths: [
        "SDK/CHeaders/XPLM",
        project.libDir,
    ]

    cpp.cxxFlags: base.concat([
                          "-Wno-unused-parameter",
                          "-Wno-unused-variable",
    ])
    cpp.cFlags: base.concat([
        "-Wno-unused-parameter",
    ])



    files: [
        "plugin.cpp",
    ]

    Group {
        name: "Shared"
        prefix: project.libDir+"/"
        files: [
            "Mandala.cpp", "Mandala.h",
            "MandalaCore.cpp", "MandalaCore.h",
            "MandalaVars.h",
            "tcp_client.cpp", "tcp_client.h",
            "tcp_server.cpp", "tcp_server.h",
            "crc.h",
            "fifo.h",
            "dmsg.h",
            "node.h",
            "time_ms.h",
        ]
    }

    Group {
        name: "SDK"
        files: [
            "SDK/CHeaders/XPLM/*.h",
        ]
    }
}

import qbs.FileInfo
import apx.Node as Node

Node.ProjectBase {

    name: "shiva"
    nodeName: "nav"
    hardware: "SIM"
    arch: undefined
    nodeDirectory: path

    modules: [
        "dlink",
        "ports",
        "shiva",
    ]

    files: [
        "main.cpp",
        "drv_sim.cpp", "drv_sim.h", "drv_sim_conf.h",
        "files_sys.cpp", "files_sys.h",
        "config.h",
        "drv_rec.h",
        "if_comm.h",
        "if_tcp_client.h",
    ]


    Node.Firmware {
        name: project.name
        targetName: name

        Group {
            name: "Modules"
            files: modules.files.concat(drivers.files)
        }
        Group {
            name: project.nodeName
            prefix: project.nodeDirectory+"/"
            files: project.files
        }
        Group {
            name: project.nodeName+".lib"
            prefix: project.libDir+"/"
            files: [
                "tcp_client.cpp",
                "tcp_client.h",
                "comm.cpp",
                "comm.h",
            ]
        }


        cpp.minimumMacosVersion: apx.minimumMacosVersion
        cpp.minimumWindowsVersion: apx.minimumWindowsVersion

        cpp.includePaths: [
            FileInfo.joinPaths(nodeDirectory,project.hardware),
            nodeDirectory,
            driversDir,
            modulesDir,
        ]
        .concat(base)
        .concat(modules.includes.concat(drivers.includes))
        .concat([project.libDir])

        cpp.defines: [
            "USE_FLOAT_TYPE",
            "AHRS_FREQ=100",
        ]
        .concat(base)
        .concat(modules.defines.concat(drivers.defines))


        Depends { name: "apx_version" }
        Properties {
            condition: qbs.targetOS.contains("linux")
            cpp.dynamicLibraries: "pthread"
        }

        //INSTALL binary
        Group {
            fileTagsFilter: [ "application" ]
            qbs.install: true
            qbs.installDir: apx.app_bin_path
            qbs.installSourceBase: product.destinationDirectory
        }
    }

}

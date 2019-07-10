import qbs.FileInfo
import apx.Application as APX

APX.ApxLibrary {


    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [
            project.libDir,
            FileInfo.joinPaths(project.libDir, "include"),
        ]
    }

    Depends { name: "apx" }
    Depends { name: "cpp" }
    Depends { name: "sdk" }

    cpp.includePaths: [
        project.libDir,
        FileInfo.joinPaths(project.libDir, "include"),
    ]

    cpp.defines: ["MANDALA_VMVARS"]

    Group {
        name: "Mandala"
        prefix: FileInfo.joinPaths(project.libDir, name, "/")
        files: [
            "Mandala.cpp", "Mandala.h",
            "MandalaCore.cpp", "MandalaCore.h",
            "MatrixMath.cpp", "MatrixMath.h",
            "MandalaTemplate.h",
            "MandalaIndexes.h",
            "MandalaConstants.h",
            "preprocessor.h",
        ]
    }
    Group {
        name: "include"
        prefix: FileInfo.joinPaths(project.libDir, name, "/")
        files: [
            "*.h",
        ]
    }
    Group {
        name: "Xbus"
        prefix: FileInfo.joinPaths(project.libDir, name, "/")
        files: [
            "xbus.h",
            "xbus_vehicle.h",
            "xbus_node.h",
            "xbus_node_conf.h",
            "xbus_node_file.h",
            "xbus_node_blackbox.h",
            "xbus_mission.h",
            "escaped.h",
        ]
    }
    Group {
        name: "Math"
        prefix: FileInfo.joinPaths(project.libDir, name, "/")
        files: [
            "crc.c", "crc.h",
        ]
    }

}

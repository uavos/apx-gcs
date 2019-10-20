import qbs
import ApxApp

import qbs.FileInfo

//Project {

ApxApp.ApxPlugin {
    id: _plugin

    Depends {
        name: "Qt";
        submodules: [
            "core",
            "multimedia",
            "qml"
        ]
    }


    files: [
        "StreamingPlugin.h",
        "gstplayer.cpp",
        "gstplayer.h",
        "overlay.cpp",
        "overlay.h",
        "videothread.cpp",
        "videothread.h",
    ]

    Depends { name: "qmlqrc" }

    Properties {
        condition: qbs.targetOS.contains("macos")
        cpp.frameworkPaths: ["/Library/Frameworks"]
        cpp.frameworks: ["GStreamer"]
        cpp.includePaths: outer.concat(["/Library/Frameworks/GStreamer.framework/Headers"])
    }

    Properties {
        condition: qbs.targetOS.contains("linux")
        cpp.includePaths: outer.concat(["/usr/include/gstreamer-1.0", //TODO: pkg-config
                                        "/usr/include/glib-2.0",
                                        "/usr/lib/x86_64-linux-gnu/gstreamer-1.0/include",
                                        "/usr/lib/x86_64-linux-gnu/glib-2.0/include",
                                       ])
        cpp.dynamicLibraries: ["gstapp-1.0", "gstrtp-1.0", "gstbase-1.0", "gstreamer-1.0", "gobject-2.0", "glib-2.0"]
    }
}

    //GStreamer
    /*ApxDeploy.ApxDeployRule {
            condition: qbs.targetOS.contains("linux") && qbs.buildVariant.contains("release")
            prepare: {
                var cmds=[];
                var targetPath = FileInfo.joinPaths(product.moduleProperty("qbs","installRoot"), product.moduleProperty("app","app_bundle_path"));

                var cmd = new Command("mkdir", ["-p", targetPath + "/lib/x86_64-linux-gnu/"])
                cmd.description = "Preparing for GStreamer deploy..."
                cmd.silent = false;
                cmds.push(cmd);

                cmd = new Command("cp",
                                  [
                                      "-r",
                                      "/usr/lib/x86_64-linux-gnu/gstreamer-1.0",
                                      targetPath + "/lib/x86_64-linux-gnu/"
                                  ]);
                cmd.description = "Deploying GStreamer plugins..."
                cmd.silent = false;
                cmds.push(cmd);

                var cmd = new Command("cp",
                                      [
                                          "-r",
                                          "/usr/lib/x86_64-linux-gnu/gstreamer1.0",
                                          targetPath + "/lib/x86_64-linux-gnu/"
                                      ]);
                cmd.description = "Deploying GStreamer binary helpers..."
                cmd.silent = false;
                cmds.push(cmd);

                var cmd = new Command("cp",
                                      [
                                          "-r",
                                          "/usr/lib/x86_64-linux-gnu/gio",
                                          targetPath + "/lib/x86_64-linux-gnu/"
                                      ]);
                cmd.description = "Deploying gio extra modules..."
                cmd.silent = false;
                cmds.push(cmd);

                return cmds;
            }
        }
    }
}*/

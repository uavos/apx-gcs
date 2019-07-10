import qbs
import qbs.File
import qbs.FileInfo
import qbs.Process

ApxDeployLibs {
    name: "DeployQt"

    property var qtplugins: []

    property bool debug: true

    Depends { name: "Qt.core" }

    outputs: qbs.targetOS.contains("macos")
        ? [ "Contents/Resources/qt.conf" ]
        : [ "bin/qt.conf" ]

    ApxDeployRule {
        prepare: {
            var cmds = []
            var deployqt
            var appPluginsDir
            var appDeployName
            var qtplugins=product.qtplugins
            var env = []


            var installRoot = product.moduleProperty("qbs","installRoot")
            var app_bundle_path = FileInfo.joinPaths(installRoot, product.moduleProperty("app","app_bundle_path"))
            var app_plugin_path = FileInfo.joinPaths(installRoot, product.moduleProperty("app","app_plugin_path"))
            var packages_dir = FileInfo.joinPaths(installRoot, product.moduleProperty("app","packages_dir"))
            var app_id = product.moduleProperty("app","app_id")
            var qt_bin_path = product.moduleProperty("Qt.core", "binPath")

            var work_dir = FileInfo.path(app_bundle_path)

            var targetOS = product.moduleProperty("qbs", "targetOS")
            if (targetOS.contains("macos")) {
                deployqt = FileInfo.joinPaths(qt_bin_path, "macdeployqt")
                appDeployName = "./"+FileInfo.fileName(app_bundle_path)
            } else if (targetOS.contains("linux")) {
                deployqt = FileInfo.joinPaths(qt_bin_path, "linuxdeployqt")
                appDeployName = FileInfo.joinPaths(app_bundle_path,"share/applications", app_id+".desktop")
            } else if (targetOS.contains("windows")) {
                deployqt = FileInfo.joinPaths(qt_bin_path, "windeployqt")
                appDeployName = FileInfo.fileName(app_bundle_path)
            }

            var opts=[]
            opts.push(appDeployName)
            opts.push("-qmldir="+FileInfo.joinPaths(project.sourceDirectory,"src"))

            var plugins=[]
            plugins=File.directoryEntries(app_plugin_path, File.Dirs)
            for(var i in plugins){
                var p=FileInfo.joinPaths(app_plugin_path, plugins[i])
                var suffix=FileInfo.completeSuffix(plugins[i])
                if(suffix=="bundle"){
                    p=FileInfo.joinPaths(p, "Contents/MacOS", FileInfo.completeBaseName(p))
                }else if (suffix=="so"){
                }else continue
                if(!File.exists(p)){
                    throw("File not found: "+p)
                }
                opts.push("-executable="+p)
                //console.info(p);
            }

            if (targetOS.contains("linux")) {
                opts.push("-unsupported-allow-new-glibc")
                opts.push("-no-copy-copyright-files")
                opts.push("-no-translations")
                opts.push("-bundle-non-qt-libs")
                opts.push("-extra-plugins="+qtplugins.join(','))
                opts.push("-qmake="+FileInfo.joinPaths(qt_bin_path, "qmake"))
                //opts.push("-verbose=2")
                opts.push("-appimage")
                env.push("ARCH="+project.arch)
                env.push("VERSION="+product.moduleProperty("git","probe").version)
            }



            if (product.debug) {
                console.info("------------------------------")
                console.info("deployqt: " + deployqt);
                console.info("work_dir: " + work_dir);
                console.info("app_bundle_path: " + app_bundle_path);
                console.info("app_plugin_path: " + app_plugin_path);
                for(var o in opts)console.info("opt: " + opts[o]);
            }
            var cmd = new Command(deployqt, opts);
            cmd.silent = false
            cmd.description = "Deploying Qt Libs..."
            cmd.workingDirectory=work_dir
            cmd.environment=env
            cmds.push(cmd);

            //extra plugins copy
            if (targetOS.contains("macos")) {
                var pluginDir = product.moduleProperty("Qt.core", "pluginPath");
                for(var i in qtplugins){
                    var p=qtplugins[i]
                    var files=File.directoryEntries(FileInfo.joinPaths(pluginDir,p),File.Files)
                    //console.info(p)
                    for(var j in files){
                        var f=files[j]
                        if(FileInfo.completeSuffix(f)!="dylib")continue
                        if(f.indexOf("debug")>=0)continue
                        //console.info(f)
                        var cmd = new JavaScriptCommand();
                        cmd.src = FileInfo.joinPaths(pluginDir,p,f)
                        cmd.dst = FileInfo.joinPaths(app_plugin_path,p,f)
                        cmd.silent = true;
                        cmd.sourceCode = function () { File.copy(src, dst); };
                        cmds.push(cmd);
                    }
                }
            }else if (targetOS.contains("linux")) {
                var cmd = new JavaScriptCommand()
                cmd.description = "Installing AppImage..."
                cmd.src = work_dir
                cmd.dst = FileInfo.joinPaths(work_dir, "packages")
                cmd.silent = false;
                cmd.sourceCode = function () {
                    File.makePath(dst)
                    files = File.directoryEntries(src, File.Files)
                    for(var i in files){
                        f = files[i]
                        if(!f.endsWith(".AppImage")) continue
                        console.info(f)
                        File.move(FileInfo.joinPaths(src, f), FileInfo.joinPaths(dst, f))
                    }
                }
                cmds.push(cmd)
            }

            return cmds;
        }
    }

}

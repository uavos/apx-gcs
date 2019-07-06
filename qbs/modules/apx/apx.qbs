import qbs
import qbs.Environment
import qbs.File
import qbs.FileInfo
import qbs.TextFile
import qbs.Process
//import "ap.js" as HelperFunctions

Module {

    property string minimumMacosVersion: "10.8"
    property string minimumWindowsVersion: "6.1"

    property string copyright_year: git.year
    property string copyright_string: "(C) " + copyright_year + " Aliaksei Stratsilatau <sa@uavos.com>"

    property string bundle_identifier: 'com.uavos.apx'

    property string app_display_name: 'APX Ground Control'
    property string app_id: 'gcs'

    property string packages_path: "packages"

    qbs.installPrefix: ""

    property bool macos: project.qbs.targetOS.contains("macos")
    property bool linux: project.qbs.targetOS.contains("linux")
    property bool windows: project.qbs.targetOS.contains("windows")

    property string app_dest_path: {
        if (macos)
            return "Applications"
        if (linux)
            return "usr"
        return ""
    }
    property string app_bundle_path: {
        if (macos)
            return FileInfo.joinPaths(app_dest_path, app_display_name+".app")
        return app_dest_path
    }

    property string app_bin_path: {
        if (macos)
            return FileInfo.joinPaths(app_bundle_path, "Contents/MacOS")
        if (linux)
            return FileInfo.joinPaths(app_bundle_path, "bin")
        return app_bundle_path
    }
    property string app_library_path: {
        if (macos)
            return FileInfo.joinPaths(app_bundle_path, "Contents/Frameworks")
        if (linux)
            return FileInfo.joinPaths(app_bundle_path, "lib", app_id)
        return app_bundle_path
    }
    property string app_plugin_path: {
        if (macos)
            return FileInfo.joinPaths(app_bundle_path, "Contents/PlugIns")
        if (linux)
            return FileInfo.joinPaths(app_bundle_path, "lib", app_id, "plugins")
        return FileInfo.joinPaths(app_bundle_path, "Plugins")
    }
    property string app_data_path: {
        if (macos)
            return FileInfo.joinPaths(app_bundle_path, "Contents/Resources")
        if (linux)
            return FileInfo.joinPaths(app_bundle_path, "share", app_id)
        return FileInfo.joinPaths(app_bundle_path, "Resources")
    }


    property var git: _git

    Probe {
        id: _git
        property string identity
        property string version
        property string branch
        property string hash
        property string time
        property string year
        property string git_top: project.sourceDirectory
        property var mod: File.lastModified(git_top + "/.git/logs/HEAD")
        property string projectName: project.name
        configure: {
            version = "0.0.0";
            branch = "";
            year = "2000";
            if(File.exists(git_top+"/.git")){
                var p = new Process();
                p.throwOnError=true
                p.setWorkingDirectory(git_top)
                if(p.exec("git", ["describe", "--always", "--tags"])===0){
                    identity = p.readStdOut().trim();
                }
                if(p.exec("git", ["describe", "--always", "--tags", "--match", "v*.*"])===0){
                    version = p.readStdOut().trim().replace("v","").replace(/-/g,".").split(".",3).join(".");
                }
                if(p.exec("git", ["rev-parse", "--abbrev-ref", "HEAD"])===0){
                    branch = p.readStdOut().trim();
                }
                if((branch=="HEAD" || branch=="") && p.exec("git", ["branch", "--contains", "HEAD"])===0){
                    //find branch on detached HEAD
                    branch=""
                    branches=p.readStdOut().trim().replace("*","").replace(/ /g,"").split("\n")
                    for(i in branches)
                    {
                        b=branches[i].trim()
                        if(b.startsWith("(")) continue
                        if(b.startsWith("HEAD")) continue
                        branch=b
                        break
                    }
                }
                if(branch=="") branch = "master";

                if(p.exec("git", ["rev-parse", "--short", "HEAD"])===0){
                    hash = p.readStdOut().trim();
                }
                if(p.exec("git", ["show", "--oneline", "--format=%ci", "-s", "HEAD"])===0){
                    time = p.readStdOut().trim();
                    year=time.split("-")[0]
                }
            }
            console.info("Project: ["+projectName+"]\tv"+version+" ("+[branch,hash,time,year].join(', ')+")")
            /*console.info("Version: "+version)
            console.info("Branch: "+branch)
            console.info("Hash: "+hash)
            console.info("Time: "+time)
            console.info("Year: "+year)*/
        }
    }


}

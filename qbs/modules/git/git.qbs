import qbs.File
import qbs.FileInfo
import qbs.Process

Module {
    property var probe: _probe

    Probe {
        id: _probe
        property string identity
        property string version
        property string branch
        property string hash
        property string time
        property string year
        property string git_top: FileInfo.joinPaths(project.sourceDirectory, "../")
        property var mod: File.lastModified(git_top + "/.git/logs/HEAD")
        property string projectName: project.name
        configure: {
            version = "0.0.0";
            branch = "";
            year = "";
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

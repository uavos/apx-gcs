import qbs
import qbs.File
import qbs.FileInfo

ApxDeployLibs {
    name: "Deploy"+framework

    property string framework

    property string fwFileName: framework+".framework"
    property string frameworkPath: "/Library/Frameworks"

    outputs: [ "Contents/Frameworks/"+fwFileName ]

    condition: qbs.targetOS.contains("macos") && qbs.buildVariant.contains("release")

    ApxDeployRule {
        prepare: {
            var cmds=[];
            var targetPath = FileInfo.joinPaths(product.moduleProperty("qbs","installRoot"), product.moduleProperty("app","app_bundle_path"));
            var cmd = new JavaScriptCommand();
            cmd.src = FileInfo.joinPaths(product.frameworkPath,product.fwFileName)
            cmd.dst = FileInfo.joinPaths(targetPath,"Contents/Frameworks",product.fwFileName)
            cmd.silent = true;
            cmd.sourceCode = function () { File.copy(src, dst); };
            cmds.push(cmd);

            return cmds;
        }
    }

}

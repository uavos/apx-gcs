import qbs
import qbs.FileInfo
import apx.Deploy as Deploy

Project {

    condition: qbs.buildVariant.contains("release")

    property string arch: "x86_64"

    Deploy.ApxDeployQt {
        Depends {
            productTypes: [
                "xplane_plugin",
            ]
        }

        qtplugins: {
            var v=[]
            v.push("texttospeech")
            if(qbs.targetOS.contains("linux")){
                v.push("geoservices/libqtgeoservices_itemsoverlay.so")
            }
            return v
        }
    }

    AppleApplicationDiskImage {
        condition: qbs.targetOS.contains("macos")

        Depends { name: "apx" }

        Depends {
            productTypes: [
                "deployment",
            ]
        }

        Group {
            fileTagsFilter: [ "dmg.dmg" ]
            qbs.install: true
            qbs.installSourceBase: product.destinationDirectory
            qbs.installDir: apx.packages_path
        }

        Rule{
            multiplex: true
            explicitlyDependsOn: ["deployment"]
            outputFileTags: [ "dmg.input" ]
            outputArtifacts: {
                var v= [{
                    filePath: FileInfo.joinPaths(product.moduleProperty("qbs","installRoot"),product.moduleProperty("apx","app_bundle_path")),
                    fileTags: [ "dmg.input" ],
                }]
                return v
            }
            prepare: {
                var cmd = new Command("echo",[outputs]);
                cmd.silent = true;
                return [cmd];
            }
        }

        name: "DmgImage"
        targetName: apx.app_display_name.replace(/ /g,"_")+"-"+apx.git.version+"-"+project.arch
        dmg.badgeVolumeIcon: true
        dmg.volumeName: apx.app_display_name+" ("+apx.git.version+")"
        dmg.iconSize: 128
        dmg.windowWidth: 640
        dmg.windowHeight: 280
        property int iposX: dmg.iconSize*1.5
        property int iposY: dmg.iconSize
        dmg.iconPositions: [
            {"path": "Applications", "x": 0*iposX, "y": iposY},
            {"path": apx.app_display_name+".app", "x": 1*iposX, "y": iposY}
        ]
        files: [
            resorcesDir+"/icons/uavos-logo.icns"
        ]
    }

}

import qbs.FileInfo

AppleApplicationDiskImage {
    condition: false //qbs.targetOS.contains("macos")
    name: "DMG"

    Depends { name: "app" }
    Depends { name: "git" }

    Depends {
        productTypes: [
            "deployment",
        ]
    }

    Group {
        fileTagsFilter: [ "dmg.dmg" ]
        qbs.install: true
        qbs.installSourceBase: product.destinationDirectory
        qbs.installDir: app.packages_path
    }

    Rule{
        multiplex: true
        explicitlyDependsOn: ["deployment"]
        outputFileTags: [ "dmg.input" ]
        outputArtifacts: {
            var v= [{
                filePath: FileInfo.joinPaths(product.moduleProperty("qbs","installRoot"),product.moduleProperty("app","app_bundle_path")),
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

    targetName: app.app_display_name.replace(/ /g,"_")+"-"+git.probe.version+"-"+project.arch
    dmg.badgeVolumeIcon: true
    dmg.volumeName: app.app_display_name+" ("+git.probe.version+")"
    dmg.iconSize: 128
    dmg.windowWidth: 640
    dmg.windowHeight: 280
    property int iposX: dmg.iconSize*1.5
    property int iposY: dmg.iconSize
    dmg.iconPositions: [
        {"path": "Applications", "x": 0*iposX, "y": iposY},
        {"path": app.app_display_name+".app", "x": 1*iposX, "y": iposY}
    ]
    files: [
        resorcesDir+"/icons/uavos-logo.icns"
    ]
}

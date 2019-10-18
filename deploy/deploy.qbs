
Project {

    condition: qbs.buildVariant.contains("release")

    references: [
        //"deploy_qt.qbs",
        "deploy_appdata.qbs",
        "deploy_sdk.qbs",
    ]
    //.concat(qbs.targetOS.contains("macos")?["deploy_dmg.qbs"]:[])
}

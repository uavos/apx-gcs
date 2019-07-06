Project {

    condition: qbs.buildVariant.contains("release")

    references: [
        "deploy_qt.qbs",
        "deploy_dmg.qbs",
        "deploy_sdk.qbs",
    ]
}

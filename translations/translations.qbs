
Project {

    condition: qbs.buildVariant.contains("release")

    references: [
        //"deploy_qt.qbs",
    ]
}

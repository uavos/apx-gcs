import qbs

Group {
    property string src
    name: "Resources."+src
    qbs.install: true
    qbs.installDir: app.app_data_path
    qbs.installSourceBase: project.resorcesDir
    prefix: project.resorcesDir+"/"+src+"/"
    excludeFiles: [
        "**/.DS_Store",
    ]
}

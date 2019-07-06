import qbs

Group {
    qbs.install: true
    qbs.installDir: apx.app_data_path
    qbs.installSourceBase: project.resorcesDir
    prefix: project.resorcesDir+"/"+name+"/"
    excludeFiles: [
        "**/.DS_Store",
    ]
}

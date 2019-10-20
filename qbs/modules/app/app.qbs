import qbs
import qbs.Environment
import qbs.File
import qbs.FileInfo

Module {

    Depends { name: "git" }

    property string version: git.probe.version

    property string minimumMacosVersion: "10.8.0"
    property string minimumWindowsVersion: "6.1"

    property string copyright_year: git.probe.year
    property string copyright_string: "(C) " + copyright_year + " Aliaksei Stratsilatau <sa@uavos.com>"

    property string bundle_identifier: 'com.uavos.apx'

    property string app_display_name: 'APX Ground Control'
    property string app_id: 'gcs'

    property string packages_path: "packages"

    qbs.installPrefix: ""

    property bool macos: project.qbs.targetOS.contains("macos")
    property bool linux: project.qbs.targetOS.contains("linux")
    property bool windows: project.qbs.targetOS.contains("windows")

    property string app_dest_path: "app"

    property string app_bundle_path: {
        if (macos)
            return FileInfo.joinPaths(app_dest_path, app_display_name+".app")
        if(linux)
            return FileInfo.joinPaths(app_dest_path, "usr")
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

}

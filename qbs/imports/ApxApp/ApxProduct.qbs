import qbs.FileInfo
import qbs.File

Product {

    property bool install: true
    property string targetInstallDir
    property var infoPlist: {}

    version: git.probe.version

    Depends { name: "app" }
    Depends { name: "cpp" }
    Depends { name: "bundle" }

    Depends { name: "appdata" } //export frameworks.json

    //qt creator support for qml files lookup
    property pathList qmlImportPaths: File.exists(sourceDirectory+"/qml")?["qml"]:[]

    //BUNDLE
    //bundle.bundleName: app.app_display_name+"."+bundle.extension
    bundle.identifierPrefix: app.bundle_identifier
    bundle.infoPlist: {
        var obj={"NSHumanReadableCopyright": app.copyright_string}
        for (var p in infoPlist) { obj[p] = infoPlist[p]; }
        return obj
    }

    //INSTALL package
    Group {
        //condition: install
        fileTagsFilter: bundle.isBundle?[ "bundle.content" ]:product.type
        qbs.install: product.install
        qbs.installDir: product.targetInstallDir
        qbs.installSourceBase: product.destinationDirectory
    }



    // NativeBinarys
    property bool isForAndroid: qbs.targetOS.contains("android")
    property bool isForDarwin: qbs.targetOS.contains("darwin")
    property bool isForLinux: qbs.targetOS.contains("linux")

    type: (isForAndroid && !consoleApplication)
        ? ["dynamiclibrary", "android.nativelibrary"]
        : ["application" ]

    aggregate: {
        if (!isForDarwin)
            return false;
        var archs = qbs.architectures;
        if (archs && archs.length > 1)
            return true;
        var variants = qbs.buildVariants;
        return variants && variants.length > 1;
    }
    multiplexByQbsProperties: {
        if (isForDarwin)
            return ["profiles", "architectures", "buildVariants"];
        if (isForAndroid)
            return ["architectures"]
        return ["profiles"];
    }

    // compiler flags
    cpp.minimumMacosVersion: app.minimumMacosVersion
    cpp.minimumWindowsVersion: app.minimumWindowsVersion

    cpp.cxxLanguageVersion: "c++14"
    cpp.useCxxPrecompiledHeader: true
    cpp.cLanguageVersion: "c11"
    //cpp.visibility: "minimal"

    //eliminate no debug symbols in executable warning
    cpp.separateDebugInformation: qbs.buildVariant != "release"


    cpp.defines: {
        var v=base
        if(qbs.toolchain.contains("msvc"))
            v.push("_CRT_SECURE_NO_WARNINGS")
        if (qbs.enableDebugCode)
            v.push("QT_STRICT_ITERATORS");
        return v
    }


    cpp.sonamePrefix: isForDarwin ? "@rpath" : undefined

    cpp.linkerFlags: {
        var flags = base;
        if (qbs.buildVariant == "debug" && qbs.toolchain.contains("msvc"))
            flags.push("/INCREMENTAL:NO"); // Speed up startup time when debugging with cdb
        if (qbs.targetOS.contains("macos"))
            flags.push("-compatibility_version", git.probe.version);
        return flags;
    }


    /*Properties {
        condition: qbs.toolchain.contains("gcc") && !qbs.toolchain.contains("clang")
        cpp.cxxFlags: base.concat(["-Wno-noexcept-type"])
    }
    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cxxFlags: base.concat(["/w44996"])
    }*/
}

import qbs
import qbs.FileInfo
import qbs.File

Product {

    property bool install: true
    property string targetInstallDir
    property var infoPlist: {}

    version: apx.git.version

    Depends { name: "apx" }
    Depends { name: "cpp" }
    Depends { name: "bundle" }

    //qt creator support for qml files lookup
    property pathList qmlImportPaths: File.exists(sourceDirectory+"/qml")?["qml"]:[]

    //BUNDLE
    //bundle.bundleName: apx.app_display_name+"."+bundle.extension
    bundle.identifierPrefix: apx.bundle_identifier
    bundle.infoPlist: {
        var obj={"NSHumanReadableCopyright": apx.copyright_string}
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



    // NativeBinary
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
    cpp.minimumMacosVersion: apx.minimumMacosVersion
    cpp.minimumWindowsVersion: apx.minimumWindowsVersion

    cpp.cxxLanguageVersion: "c++14"
    cpp.useCxxPrecompiledHeader: true
    cpp.cLanguageVersion: "c11"
    //cpp.visibility: "minimal"


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
            flags.push("-compatibility_version", apx.git.version);
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

import apx.Deploy as Deploy

Deploy.ApxDeployQt {
    name: "Qt Libs"

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

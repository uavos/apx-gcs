import QtQuick 2.12
import QtQml 2.12

Loader {
    active: false
    asynchronous: false
    source: {
        var cname="MenuBar"
        if(Qt.platform.os=="linux")cname+="Linux"
        else cname+="Macos"
        return cname+".qml"
    }
    Connections {
        target: application
        onLoadingFinished: active=true
    }

}

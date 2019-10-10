import QtQuick 2.12
import QtQml 2.12

import APX.Facts 1.0

Loader {
    asynchronous: false
    source: {
        var cname="MenuBar"
        if(Qt.platform.os=="linux")cname+="Linux"
        else cname+="Macos"
        return cname+".qml"
    }
}

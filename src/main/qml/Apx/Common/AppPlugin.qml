import QtQuick 2.5
import QtQml 2.12

Loader {
    id: plugin
    property string uiComponent
    signal configure()

    active: false
    asynchronous: true
    Component.onCompleted: {
        if(uiComponent && (!(ui && ui[uiComponent])))return
        activate()
    }
    function activate()
    {
        configure()
        active=true
    }
    Connections {
        target: application
        enabled: uiComponent
        onUiComponentLoaded: if(name==plugin.uiComponent)activate()
    }
}

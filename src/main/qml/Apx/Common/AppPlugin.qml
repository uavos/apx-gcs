import QtQuick 2.5
import QtQml 2.12

Loader {
    id: plugin
    property string uiComponent
    signal configure()  // called when uiComponent is loaded

    property var fact
    property string name: fact?fact.name:""
    property string title: fact?fact.title:""
    property string descr: fact?fact.descr:""
    property string icon: fact?fact.icon:""


    active: false
    asynchronous: true
    Component.onCompleted: {
        if(uiComponent && (!(ui && ui[uiComponent])))return
        activate()
    }

    function activate(object)
    {
        configure()
        active=true
    }
    Connections {
        target: application
        enabled: uiComponent
        onUiComponentLoaded: if(name==plugin.uiComponent)activate(object)
    }
}

import QtQuick 2.5
import QtQml 2.12

Loader {
    id: plugin
    property string uiComponent
    signal configure()  // called when uiComponent is loaded

    property bool unloadOnHide: true

    property var fact   //set by C++ Fact->loadQml()
    property string name: fact?fact.name:""
    property string title: fact?fact.title:""
    property string descr: fact?fact.descr:""
    property string icon: fact?fact.icon:""

    visible: true

    active: false
    asynchronous: true
    Component.onCompleted: {
        if(uiComponent && (!(ui && ui[uiComponent])))return
        activate()
    }

    function activate(object)
    {
        active=unloadOnHide?Qt.binding(function(){return visible}):true
        configure()
    }
    Connections {
        target: application
        enabled: uiComponent
        onAboutToQuit: plugin.active=false
        onUiComponentLoaded: if(name==plugin.uiComponent)activate(object)
    }
    //onLoaded: console.log(plugin)
}

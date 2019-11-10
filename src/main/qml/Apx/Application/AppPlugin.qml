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

    //forward reference to adjust view states of widget, i.e. "maximized", "minimized"
    readonly property string pluginState: state
    readonly property bool pluginMinimized: pluginState=="minimized"


    visible: true

    active: false
    asynchronous: true
    Component.onCompleted: {
        if(uiComponent && (!(ui && ui[uiComponent])))return
        activate()
    }

    function activate(object)
    {
        active=true //unloadOnHide?Qt.binding(function(){return visible}):true
        configure()
    }
    Connections {
        target: application
        enabled: uiComponent
        onAboutToQuit: {
            plugin.visible=false
            plugin.active=false
            plugin.sourceComponent=null
            plugin.uiComponent=""
        }
        onUiComponentLoaded: if(name==plugin.uiComponent)activate(object)
    }
    //onLoaded: console.log(plugin)
}

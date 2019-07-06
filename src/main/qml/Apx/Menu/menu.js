.pragma library
.import QtQuick.Controls 2.4 as Controls
.import QtQml 2.11 as Qml

var component
var popup=null
var options={}

function show(fact,opts,parent)
{
    if(popup && typeof(popup.pinned)!=='undefined'){
        if(!popup.pinned){
            popup.close()
            popup=null
        }
    }

    if(!opts)opts={}
    if(!parent)parent=ui.window //Controls.ApplicationWindow.contentItem
    //console.log(fact,opts)

    options=opts
    options.fact=fact
    options.parentObject=parent

    component = Qt.createComponent("FactMenuPopup.qml",Qml.Component.Asynchronous,parent);
    if (component.status === Qml.Component.Ready){
        createMenuObject(component);
    }else{
        component.statusChanged.connect(createMenuObject);
    }
}

function createMenuObject()
{
    if (component.status === Qml.Component.Error) {
        console.log("Error loading component:", component.errorString());
        return
    }
    if (component.status !== Qml.Component.Ready) return;

    popup = component.createObject(options.parentObject, options);
    /*if (popup.status !== Qml.Component.Ready) {
        popup.onStatusChanged = function(status) {
            if (status === Qml.Component.Ready) {
                createPopup();
            }
        }
    } else {
        createPopup();
    }*/
    createPopup()
}

function createPopup()
{
    if (popup === null || popup.status === Qml.Component.Error) {
        console.log("Error creating object:", popup.errorString());
        return
    }
    //ui.popup=popup
    //console.log(options.fact)
    popup.open()
    popup.closed.connect(function(){
        //delete ui.popup
        //popup=null
    })
}

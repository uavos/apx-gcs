import QtQuick 2.6
import QtQuick.Controls 2.1
import QtGraphicalEffects 1.0
import "."

Popup {
    id: root
    //modal: true
    //focus: true
    parent: window

    property FactMenu menu
    property string source

    //contentWidth: Math.min(window.width, window.height) / 3 * 2
    //contentHeight: Math.min(window.height, contents.length*menu.height)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    contentHeight: 12*menu.itemSize+menu.itemSize*1.2
    contentWidth: menu.itemSize*10 //cWidth

    padding: 0

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 0.9 }
    }


    onClosed: menu.close()

    //contentItem: menu

    Component.onCompleted:{
        if(menu){
            menu.closeable=true
            menu.closed.connect(root.close)
            menu.opened.connect(root.open)
            contentItem=menu
        }
    }

    onOpened: {
        if(source){
            var c=Qt.createComponent(source,Component.Asynchronous,root);
            c.statusChanged.connect( function(status) {
                if (status === Component.Ready) {
                    root.contentItem=c.createObject(root);
                }
            })
            c.statusChanged(c.status);
        }
    }

}


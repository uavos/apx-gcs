import QtQuick 2.6
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import GCS.FactSystem 1.0
import "../components"
import "."

ColumnLayout {
    id: menuPage

    //Fact bindings
    property var fact

    property bool pageInfo: false //load fact info instead of list

    //implicitHeight: 500 //childrenRect.height
    //implicitWidth: 300
    //Component.onDestruction: console.log("page delete: "+fact.title)
    StackView.onRemoved: destroy()

    Connections {
        target: fact
        onRemoved: {
            fact=dummyFactC.createObject(menuPage);
            destroy()
        }
    }
    Component {
        id: dummyFactC
        FactMenuElement { }
    }

    property int padding: 4
    clip: true

    Component.onCompleted: {
        if(showTitle||true){
            loadPage("FactMenuPageTitle.qml",{
                "clip": true,
                "Layout.fillWidth": true,
                "Layout.leftMargin": padding,
                "Layout.rightMargin": padding,
                "Layout.topMargin": padding
            })
        }
        var opts={
            "clip": true,
            "Layout.fillWidth": true,
            "Layout.fillHeight": true,
            "Layout.leftMargin": padding,
            "Layout.rightMargin": padding,
            "Layout.bottomMargin": padding
        }
        if(fact.qmlPage){
            loadPage(fact.qmlPage,opts)
        }else if(pageInfo){
            loadPage("FactMenuPageInfo.qml",opts)
        }else{
            loadPage("FactMenuPageList.qml",opts)
        }
    }

    function loadPage(qmlPage,opts)
    {
        var cmp=Qt.createComponent(qmlPage,menuPage)
        if (cmp.status === Component.Ready) {
            cmp.createObject(menuPage,opts);
        }
    }
}

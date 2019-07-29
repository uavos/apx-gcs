import QtQuick 2.6
import QtQuick.Controls 2.1
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import QtQml 2.12

import APX.Facts 1.0
import Apx.Common 1.0

ColumnLayout {
    id: menuPage

    //Fact bindings
    property var fact

    property bool pageInfo: false //load fact info instead of list
    property var pageInfoAction //load fact action info instead fact info

    property string pageTitle: fact.title
    property string pageStatus: fact.status

    //implicitHeight: 500 //childrenRect.height
    //implicitWidth: MenuStyle.itemWidth
    //Component.onDestruction: console.log("page delete: "+fact.title)
    StackView.onRemoved: {
        //console.log("page removed from stack: "+fact.title)
        if(menuPage)menuPage.destroy()
    }
    StackView.onActivated: {
        //forceActiveFocus()
        //implicitHeight=0
        //implicitHeight=childrenRect.height
        /*if(autoResize){
            autoResize=false
            autoResize=true
        }*/
    }

    Connections {
        target: fact
        onActionTriggered: factActionTriggered()
        onMenuBack: back()
    }
    onFactChanged: {
        if(!fact){
            //console.log("page fact removed")
            fact=dummyFactC.createObject(menuPage);
            //factRemoved()
            if(menuPage) popItem(menuPage)
        }
    }
    Component {
        id: dummyFactC
        FactObject { }
    }

    property int padding: 4
    clip: true

    Loader {
        active: true
        asynchronous: true
        source: "FactMenuPageTitle.qml"
        Layout.fillWidth: true
        Layout.leftMargin: padding
        Layout.rightMargin: padding
        Layout.topMargin: padding
        Layout.alignment: Qt.AlignTop|Qt.AlignHCenter
        clip: true
    }
    Loader {
        id: pageLoader
        active: true
        asynchronous: true
        //source: pageSource()
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: padding
        Layout.rightMargin: padding
        Layout.topMargin: padding
        clip: true
        property var opts
        onLoaded: {
            for(i in opts){
                item[i]=opts[i]
            }
        }
    }

    function pageSource()
    {
        if(pageInfo){
            if(pageInfoAction){
                pageTitle+=": "+pageInfoAction.title
                pageLoader.opts.factAction=pageInfoAction
            }
            return "FactMenuPageInfo.qml"
        }else if(fact.qmlPage){
            var s=fact.qmlPage
            if(s.indexOf(":")>=0){
                return s
            }else{
                return "../"+s
            }
        }else if(fact.dataType===Fact.Mandala){
            pageTitle+=": "+qsTr("select")
            return "FactMenuPageMandala.qml"
        }else{
            return "FactMenuPageList.qml"
        }
    }

    Component.onCompleted: {
        var src=pageSource()
        if(src){
            pageLoader.source=src
        }
    }
}

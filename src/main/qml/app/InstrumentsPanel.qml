import QtQuick 2.11
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtQml 2.12

import Qt.labs.settings 1.0

import Apx.Common 1.0
import Apx.Controls 1.0
import Apx.Instruments 1.0
import Apx.Menu 1.0

import "qrc:/app"

RowLayout {
    id: control

    spacing: 0

    readonly property color sepColor: "#244"


    function addPlugin(plugin)
    {
        instrumentsItem.add(plugin)
        plugin.active=Qt.binding(function(){return control.visible})
        if(plugin.name)
            application.registerUiComponent(plugin,"instruments."+plugin.name)
    }

    function addComponent(c)
    {
        var obj=c.createObject(control,{"Layout.fillHeight": true})
        c_vsep.createObject(control)
        return obj
    }

    Component.onCompleted: {
        addComponent(status)
        addComponent(pfd)
        addComponent(numbers)
        addComponent(commands)

        instrumentsItem=addComponent(c_instruments)
    }
    property var instrumentsItem

    Component {
        id: status
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { Status { } }
        }
    }
    Component {
        id: pfd
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { Pfd { } }
            Layout.preferredWidth: control.height*2
        }
    }
    Component {
        id: numbers
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { NumbersBox { settingsName: "instruments" } }
        }
    }
    Component {
        id: commands
        Loader {
            active: control.visible
            asynchronous: true
            sourceComponent: Component { Commands { } }
        }
    }

    Component {
        id: c_vsep
        Rectangle {
            Layout.fillHeight: true
            implicitWidth: 1
            border.width: 0
            color: sepColor
        }
    }

    Component {
        id: c_instruments
        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 1
            function add(c)
            {
                c.parent=swipeView
                pagesModel.append(c)
            }
            ListModel {
                id: pagesModel
            }
            SwipeView {
                id: swipeView
                Layout.fillHeight: true
                Layout.fillWidth: true
                interactive: false
                orientation: Qt.Horizontal
                clip: true
            }
            ListView {
                id: listView
                Layout.alignment: Qt.AlignRight
                Layout.margins: 3
                implicitWidth: contentWidth
                spacing: 3
                model: pagesModel
                visible: count>1
                orientation: ListView.Horizontal
                delegate: CleanButton {
                    text: model.title?model.title:index
                    toolTip: model.descr
                    iconName: model.icon
                    highlighted: swipeView.currentIndex==index
                    onTriggered: swipeView.currentIndex=index
                    Component.onCompleted: {
                        listView.implicitHeight=Math.max(listView.implicitHeight, implicitHeight)
                    }
                }
            }
        }
    }
}

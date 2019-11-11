import QtQuick 2.5;
import QtQuick.Layouts 1.3

import QtQml.Models 2.11

import Apx.Common 1.0

RowLayout {
    id: control

    //warnings fact
    WarningMessage {
        Layout.fillHeight: true
        //implicitWidth: height
    }

    //widgets
    WidgetsListView {
        id: widgetsView
        Layout.fillHeight: true
    }
    function addWidgetControl(plugin, index){widgetsView.add(plugin,index)}




    //tools list from plugins
    DelegateModel {
        id: toolsModel
        model: apx.windows.model
        groups: [ DelegateModelGroup { name: "launcher" } ]
        filterOnGroup: "launcher"
        delegate: CleanButton {
            iconName: modelData.icon
            onTriggered: modelData.trigger()
            toolTip: modelData.title+" - "+modelData.descr
        }
        Component.onCompleted: sync()
        property int size: apx.windows.model.count
        onSizeChanged: sync()
        function sync()
        {
            for( var i = 0;i < items.count;i++ ) {
                var entry = items.get(i)
                if(entry.model.modelData && entry.model.modelData.showLauncher) {
                    toolsModel.items.setGroups(i--,1,"launcher")
                }
            }
        }
    }
    ListView {
        Layout.fillHeight: true
        implicitWidth: contentWidth
        spacing: 4
        orientation: ListView.Horizontal
        model: toolsModel
    }

    //tools menu
    CleanButton {
        Layout.fillHeight: true
        iconName: apx.tools.icon
        toolTip: apx.tools.title
        onTriggered: apx.tools.trigger()
    }
    //windows menu
    CleanButton {
        Layout.fillHeight: true
        iconName: apx.windows.icon
        toolTip: apx.windows.title
        onTriggered: apx.windows.trigger()
    }

    //simulator
    Loader {
        Layout.fillHeight: true
        asynchronous: true
        active: typeof(apx.tools)!=='undefined' && typeof(apx.tools.simulator)!=='undefined' && (apx.vehicles.current.isLocal() || apx.tools.simulator.stop.enabled)
        sourceComponent: Component {
            CleanButton {
                iconName: apx.tools.simulator.icon
                toolTip: apx.tools.simulator.descr
                onTriggered: apx.tools.simulator.trigger()
            }
        }
        visible: status===Loader.Ready
    }

    CleanButton {
        Layout.fillHeight: true
        iconName: groundControl.maximized?"fullscreen-exit":"fullscreen"
        toolTip: qsTr("Switch view")
        onTriggered: groundControl.toggleState()
    }

    Loader {
        Layout.alignment: Qt.AlignRight|Qt.AlignTop
        active: apx.vehicles.current.isReplay()
        visible: active
        sourceComponent: Component { TelemetryReader { } }
    }
    RecLabel {
        Layout.fillHeight: true
    }
    ClockLabel {
        Layout.fillHeight: true
        Layout.rightMargin: height*0.3
    }
    Loader {
        Layout.fillHeight: true
        active: Qt.platform.os === "linux"
        visible: active
        sourceComponent: Component {
            FactButton {
                fact: apx.sysmenu
                showText: false
            }
        }
    }
}

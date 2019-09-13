import QtQuick 2.5;
import QtQuick.Layouts 1.3

import QtQml.Models 2.11

import Apx.Common 1.0

RowLayout {
    id: control

    //warnings fact
    WarningMessage {
        Layout.fillHeight: true
    }

    //reset path
    CleanButton {
        readonly property var fact: apx.vehicles.current.action.rpath
        iconName: fact.icon
        implicitHeight: parent.height
        toolTip: fact.title
        onTriggered: fact.trigger()
        visible: fact.enabled
    }


    //tools list from plugins
    DelegateModel {
        id: toolsModel
        model: apx.windows.model
        groups: [ DelegateModelGroup { name: "launcher" } ]
        filterOnGroup: "launcher"
        delegate: CleanButton {
            iconName: modelData.icon
            implicitHeight: parent.height
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
    CleanButton {
        iconName: apx.tools.icon
        implicitHeight: parent.height
        toolTip: apx.tools.title
        onTriggered: apx.tools.requestMenu()
    }
    CleanButton {
        iconName: apx.windows.icon
        implicitHeight: parent.height
        toolTip: apx.windows.title
        onTriggered: apx.windows.requestMenu()
    }

    //simulator
    Loader {
        asynchronous: true
        active: typeof(apx.tools)!=='undefined' && typeof(apx.tools.sim)!=='undefined' && (apx.vehicles.current===apx.vehicles.LOCAL || apx.tools.sim.action.stop.enabled)
        sourceComponent: Component {
            CleanButton {
                iconName: apx.tools.sim.icon
                implicitHeight: control.height
                toolTip: apx.tools.sim.descr
                onTriggered: apx.tools.sim.requestMenu()
            }
        }
        visible: status===Loader.Ready
    }
    //telemetry player
    Loader {
        Layout.fillHeight: true
        asynchronous: true
        active: apx.vehicles.current===apx.vehicles.REPLAY
        sourceComponent: Component { TelemetryReader { } }
        visible: status===Loader.Ready
    }

    RecLabel {
        Layout.fillHeight: true
    }
    ClockLabel {
        Layout.fillHeight: true
        Layout.rightMargin: height*0.3
    }
}

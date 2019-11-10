import QtQuick 2.12
import QtGraphicalEffects 1.0

Item {
    id: control

    property int w: 2
    property int size: 100

    width: size
    height: size

    enum AimType {
        None,
        Crosshair,
        Rectangle
    }

    property int type: OverlayAim.AimType.Crosshair

    Item {
        id: content
        anchors.fill: parent
        visible: false
        Loader {
            active: control.type === OverlayAim.AimType.Crosshair
            anchors.fill: parent
            sourceComponent: Item {
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width
                    height: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: control.w
                    height: parent.height
                    border.width: 0
                    color: "#fff"
                }
            }
        }
        Loader {
            active: control.type === OverlayAim.AimType.Rectangle
            anchors.fill: parent
            sourceComponent: Item {
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    width: parent.width/4
                    height: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    height: parent.height/4
                    width: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    width: parent.width/4
                    height: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: parent.height/4
                    width: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    width: parent.width/4
                    height: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    height: parent.height/4
                    width: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    width: parent.width/4
                    height: control.w
                    border.width: 0
                    color: "#fff"
                }
                Rectangle {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: parent.height/4
                    width: control.w
                    border.width: 0
                    color: "#fff"
                }
            }
        }
    }
    DropShadow {
        anchors.fill: content
        samples: 15
        color: "#000"
        source: content
        cached: true
    }
}

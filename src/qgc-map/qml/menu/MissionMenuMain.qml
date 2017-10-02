import QtQuick 2.2
import QtGraphicalEffects 1.0
import com.uavos.map 1.0
import "."
import "../"
import "../components"


MissionMenuPage {
    caption: "CMD"
    listModel: /*ListModel {
        ListElement {
            caption: qsTr("Upload")
            icon: "android-upload"
            //page: "RW"
        }
        ListElement {
            caption: qsTr("Download")
            icon: "android-download"
            //page: "WP"
        }
    }*/
    VisualItemModel {
        Rectangle { height: 30; width: 80; color: "red" }
        Rectangle { height: 30; width: 80; color: "green" }
        Rectangle { height: 30; width: 80; color: "blue" }
    }
}

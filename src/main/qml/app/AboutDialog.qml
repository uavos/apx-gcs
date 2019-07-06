import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.4

Dialog {
    id: dialog
    title: qsTr("About")+" "+Qt.application.name
    standardButtons: Dialog.Close

    x: (parent.width-width)/2
    y: (parent.height-height)/2

    RowLayout {
        Image {
            Layout.alignment: Qt.AlignTop|Qt.AlignLeft
            Layout.preferredWidth: 128
            Layout.preferredHeight: 128
            source: "qrc:/icons/uavos-logo.ico"
            fillMode: Image.PreserveAspectFit
        }
        ColumnLayout {
            Layout.fillHeight: true
            Layout.preferredWidth: dialog.parent.width/4
            TextEdit {
                Layout.fillWidth: true
                color: Material.foreground
                text: application.aboutString()
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.WordWrap
                selectByMouse: true
                selectByKeyboard: true
                onLinkActivated: Qt.openUrlExternally(link)
            }
        }
    }
}

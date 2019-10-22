import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12

Dialog {
    id: dialog

    title: qsTr("About")+" "+Qt.application.name
    standardButtons: Dialog.Close

    property var w: parent
    x: (w.width-width)/2
    y: (w.height-height)/2


    implicitWidth: Math.max(w.width/4,800)
    //implicitHeight: Math.max(parent.height/4,100)

    contentItem: RowLayout {
        Image {
            Layout.alignment: Qt.AlignTop|Qt.AlignLeft
            Layout.preferredWidth: 128
            Layout.preferredHeight: 128
            source: "qrc:/icons/uavos-logo.ico"
            fillMode: Image.PreserveAspectFit
        }
        TextEdit {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Material.foreground
            text: application.aboutString()
            font: dialog.font
            //font.family: font_condenced
            //textFormat: TextEdit.RichText
            wrapMode: TextEdit.Wrap
            selectByMouse: true
            selectByKeyboard: true
            readOnly: true
            onLinkActivated: Qt.openUrlExternally(link)
        }
    }
}

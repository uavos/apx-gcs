import QtQuick 2.7
import QtQml 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "."
import "../components"

GCSMenu {
    id: menuPreferences
    title: qsTr("Preferences")

    fields: GCSMenuModel {
        GCSMenuField { title: qsTr("Connectivity"); separator: true; }
        GCSMenuField { fact: app.settings.readonly } //title: qsTr("Read only"); onClicked: app.settings.readonly.value=!app.settings.readonly.value; checked: app.settings.readonly.value; checkable: true; }
        GCSMenuField {
            title: qsTr("Serial Ports");
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("Add new");  }
            }
        }
        GCSMenuField {
            title: qsTr("Network");
            fields: GCSMenuModel {
                GCSMenuField { title: qsTr("Server"); separator: true; }
                GCSMenuField { title: qsTr("Host");  }
                GCSMenuField { title: qsTr("Password");  }
                GCSMenuField { title: qsTr("Allow external controls"); checkable: true; }

                GCSMenuField { title: qsTr("Client"); separator: true; }
                GCSMenuField { title: qsTr("Master Host");  }
                GCSMenuField { title: qsTr("Master Password");  }

                GCSMenuField { title: qsTr("Other"); separator: true; }
                GCSMenuField { title: qsTr("HTTP proxy");  }
            }
        }
        GCSMenuField { title: qsTr("Application"); separator: true; }
        GCSMenuField { fact: app.settings.sounds } //title: qsTr("Sounds"); onClicked: mandala.soundsEnabled=!mandala.soundsEnabled; checked: mandala.soundsEnabled; checkable: true; }
        GCSMenuField { title: "Language"; }
        GCSMenuField { title: qsTr("Voice");  }
        GCSMenuField { title: qsTr("Shortcuts"); pageMenu: "menu/MenuShortcuts.qml" }

        GCSMenuField { title: qsTr("Graphics"); separator: true; }
        GCSMenuField { fact: app.settings.opengl } //title: qsTr("Accelerate graphics"); checkable: true; }
        GCSMenuField { fact: app.settings.smooth } //title: qsTr("Smooth animations"); onClicked: app.settings.smooth.value=!app.settings.smooth.value; checked: app.settings.smooth.value; checkable: true; }
    }

}

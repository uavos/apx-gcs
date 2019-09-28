import QtQuick 2.12

import Apx.Menu 1.0

FactMenuPopup {
    id: popup
    pinned: true

    property alias defaults: menuFact.defaults
    property alias settingsName: menuFact.settingsName

    signal accepted()

    fact: menuFact
    NumbersMenu {
        id: menuFact
        onAccepted: popup.accepted()
    }
    onClosed: menuFact.destroy()
}

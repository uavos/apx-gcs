import QtQuick 2.5;

import Apx.Common 1.0

FactValue {

    property bool light: false

    //valueScale: light?0.7:1
    normalColor: light?"#555":normalColor
    //implicitHeight: 0
}

import QtQuick 2.2
import QtGraphicalEffects 1.0

import Apx.Common 1.0

SvgImage {
    id: control
    property bool glow: true
    property color glowColor: "#000"

    effect: glow?glowC:blurC

    Component {
        id: glowC
        Glow {
            id: imageGlow
            anchors.fill: control
            radius: ui.effects?4:0
            samples: 8
            color: glowColor
        }
    }
    Component {
        id: blurC
        FastBlur {
            id: imageBlur
            anchors.fill: control
            radius: ui.antialiasing?Math.max(width,height)/10:0
        }
    }
}

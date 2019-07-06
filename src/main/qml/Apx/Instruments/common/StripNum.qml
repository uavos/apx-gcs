import QtQuick 2.2

Item {
    id: stripRect
    property double value
    property double numScale: 0.5
    property double numGapScale: 1
    property color color: "white"
    property color colorNeg: "#f88"
    property bool showZero: true
    property int divider: 0


    Rectangle {
        color: "transparent" //"#5000ff00" //
        border.width: 0
        anchors.fill: parent
        clip: true
        Item {
            id: strip
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: offset(dspValue)*numStep
            property double dspValue: divider>0?(value>=0?Math.floor((value+0.4)/divider):Math.ceil((value-0.4)/divider)):value
            property double numSize: stripRect.height*stripRect.numScale
            property double numStep: numSize*stripRect.numGapScale

            Behavior on dspValue { enabled: ui.smooth && divider>0; PropertyAnimation {duration: 100; } }

            function mod(x,y) {
                return x-y*Math.floor(x/y);
            }
            function offset(v) {
                var x=mod(v,10);
                if(v<-10) x-=20;
                else if(v<0) x-=10;
                else if(v>=10)x+=10;
                return x;
            }
            Repeater {
                model: 30
                Text {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: -(index)*strip.numStep
                    text: (showZero===false && strip.dspValue<5 && strip.dspValue>-5 && (index%10)===0)?"":Math.abs(index) % 10
                    color: stripRect.color
                    font.family: font_narrow
                    font.pixelSize: strip.numSize
                }
            }
            Repeater {
                model: 30
                Text {
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: (index+1)*strip.numStep
                    text: (showZero===false && strip.dspValue<5 && strip.dspValue>-5 && ((index+1)%10)===0)?"":Math.abs(index+1) % 10
                    color: stripRect.colorNeg
                    font.family: font_narrow
                    font.pixelSize: strip.numSize
                }
            }
        }
    }
}

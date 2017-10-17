import QtQuick 2.2

Item {
    id: textLine
    property string text
    property color color: "white"

    property int showTime: 5000
    property int showLoops: 3

    visible: listModel.count>0

    function show(s,color)
    {
        if(s.toString()==="")return;
        for(var i=0;i<listModel.count;i++){
            if(listModel.get(i).text===s.toString()){
                listModel.setProperty(i,"loops",textLine.showLoops);
                listModel.setProperty(i,"color",color);
                return;
            }
        }
        listModel.append({text: s.toString(), color: color, loops: textLine.showLoops});
        //console.log(s);
        if(!nextTimer.running)showNext();
    }

    ListModel
    {
        id: listModel
        property string text
        property color color
        property int loops
    }

    property int idx: 0
    function showNext()
    {
        //console.log("cnt: "+listModel.count.toFixed() + " idx: "+idx.toFixed());
        if(listModel.count>0){
            if(idx>=listModel.count)idx=0;
            showText(listModel.get(idx).text,listModel.get(idx).color);
            var loops=listModel.get(idx).loops;
            loops--;
            listModel.setProperty(idx,"loops",loops);
            if(loops<=0)listModel.remove(idx,1);
            idx++;
            nextTimer.start();
        }else{
            textItem.text=textLine.text
            textItem.color=textLine.color
        }
    }

    function showText(s,color)
    {
        textItem.text="";
        textItem.text=s.toString();
        textItem.color=color;
    }


    Timer {
        id: nextTimer
        interval: textLine.showTime; running: false; repeat: false
        onTriggered: showNext();
    }

    Text {
        id: textItem
        anchors.fill: parent
        anchors.topMargin: 0
        anchors.rightMargin: 1
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        text: textLine.text
        font.pixelSize: height
        font.family: font_condenced
        color: textLine.color
        onTextChanged: { fallTimer.stop(); textItem.opacity=1; fallTimer.start();}
        Behavior on opacity { enabled: app.settings.smooth.value; PropertyAnimation {duration: 200} }
        Timer {
            id: fallTimer
            interval: textLine.showTime-200; running: false; repeat: false
            onTriggered: textItem.opacity=0
        }
    }
}


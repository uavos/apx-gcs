import QtQuick 2.3
import QtCharts 2.2
import QtQuick.Controls 2.2
import QtQml 2.12

Item {
    id: chartItem
    //clip: true
    property var facts: []

    property bool openGL: false //apx.settings.graphics.opengl.value
    property bool smoothLines: ui.smooth


    property real speed: 0
    property real lineWidth: ui.antialiasing?1.5:1
    property real lineWidthCmd: ui.antialiasing?2.1:2

    property var speedFactor: [ 1, 2, 4, 0.5, 0.2 ]
    property real speedFactorValue: speed<0?speedFactor[0]:speed>=speedFactor.length?speedFactor[speedFactor.length-1]:speedFactor[speed]

    onFactsChanged: {
        chartView.reset()
    }

    Connections {
        target: apx.vehicles
        onCurrentDownstreamDataReceived: chartView.appendData();
    }

    ChartView {
        id: chartView

        antialiasing: ui.antialiasing
        legend.visible: false
        margins.top: 0
        margins.left: 0
        margins.bottom: 0
        margins.right: 0

        anchors.fill: parent
        property int margin: -8
        anchors.topMargin: margin
        anchors.bottomMargin: margin
        anchors.leftMargin: margin
        anchors.rightMargin: margin
        //onPlotAreaChanged: margin=-plotArea.y/3

        plotAreaColor: "black"
        backgroundColor: "black"
        backgroundRoundness: 0
        dropShadowEnabled: false

        property int samples: Math.min(1000,Math.max(25,width/(3*speedFactorValue)))
        property int time: 0

        property bool dataExist: false

        ValueAxis {
            id: axisX
            property real t: chartView.time
            Behavior on t { enabled: ui.smooth && chartView.dataExist; NumberAnimation {duration: 500; } }
            min: t-chartView.samples+20
            max: t
            //min: -chartView.samples //t-chartView.samples+20
            //max: 0 //t
            visible: false
            gridVisible: false
            labelsVisible: false
            lineVisible: false
            shadesVisible: false
            titleVisible: false
        }
        ValueAxis {
            id: axisY
            min: -0
            max: 0
            tickCount: 4
            labelsColor: "white"
            labelsFont.pixelSize: Qt.application.font.pixelSize * 0.7
            gridLineColor: "#555"
        }


        property real dataPadding: 0.05
        property real dataPaddingZero: 0.05
        property var sdata: []
        property int timeRescale: 0

        function reset()
        {
            chartView.removeAllSeries();
            chartView.sdata=[]
            chartView.time=0
            axisY.min=-dataPaddingZero
            axisY.max=dataPaddingZero
            axisY.tickCount=4
            axisY.applyNiceNumbers()
            speed=0
        }

        function appendData()
        {
            var t=time+1;
            for(var i=0;i<facts.length;++i){
                if(i>=chartView.count)addFactSeries(facts[i])
                var s=chartView.series(i)
                var v=facts[i].value
                if(!isFinite(v))v=0
                s.append(t,v);
                sdata.push(v)
                //instant rescale - grow
                if(axisY.max<v){
                    axisY.max=v+dataPadding;
                }
                if(axisY.min>v){
                    axisY.min=v-dataPadding;
                }
                //remove old
                var cnt=samples
                if(s.count>cnt) s.removePoints(0,s.count-cnt)
            }
            //calc scale - reduce
            if((t-timeRescale)>21){
                timeRescale=t
                var d=sdata.length-samples*facts.length
                if(d>0)sdata.splice(0,d)
                var p=apx.seriesBounds(sdata)
                var min=p.x-dataPadding
                var max=p.y+dataPadding
                if(min==max){
                    min-=dataPaddingZero
                    max+=dataPaddingZero
                }
                var bmod=false
                if(axisY.min<min){
                    axisY.min=min
                    bmod=true
                }
                if(axisY.max>max){
                    axisY.max=max
                    bmod=true
                }
                if(bmod){
                    axisY.tickCount=4
                    axisY.applyNiceNumbers()
                }
            }
            time=t
            dataExist=true
        }

        function addFactSeries(fact)
        {
            var s = chartView.createSeries(ui.antialiasing?ChartView.SeriesTypeLine:ChartView.SeriesTypeLine,fact.title,axisX, axisY)
            s.useOpenGL = Qt.binding(function(){return openGL})
            s.capStyle=Qt.RoundCap
            //s.opacity=0.7

            var color = fact.opts.color
            if(!color) color = Qt.rgba(1,1,1,1)

            if(fact.name.startsWith("cmd")){
                s.width=Qt.binding(function(){return lineWidthCmd})
                s.color=Qt.hsla(color.hslHue, color.hslSaturation/2, color.hslLightness*1.2, 1)
            }else{
                s.width=Qt.binding(function(){return lineWidth})
                s.color=color
            }
            return s
        }

    }


    function changeSpeed()
    {
        if((speed+1)<speedFactor.length)speed++
        else speed=0
        //console.log(speed)
    }

    /*ToolTip {
        delay: 1000
        timeout: 5000
        visible: text && (mouseArea.containsMouse)
        text: getToolTip(facts)
        background: Rectangle {
            color: "#000"
            border.width: 0
            radius: 5
        }
    }


    function getToolTip(facts)
    {
        var s=[]
        for(var i=0;i<facts.length;++i){
            var fact=facts[i]
            s.push("<font color='"+fact.color+"'>"+fact.descr+"</font>")
        }
        return s.join("<br>")
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onDoubleClicked: changeSpeed()
        //onPressAndHold: drawer.open()
    }*/

}


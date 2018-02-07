import QtQuick 2.3
import QtCharts 2.2

Item {
    clip: true
    property var facts: []

    property bool openGL: app.settings.opengl.value
    property bool smooth: app.settings.smooth.value


    property real speed: 0
    property real lineWidth: smooth?2.5:1
    property real lineWidthCmd: smooth?5.1:3

    property var speedFactor: [ 1, 2, 4, 0.5, 0.25 ]
    property real speedFactorValue: speed<0?speedFactor[0]:speed>=speedFactor.length?speedFactor[speedFactor.length-1]:speedFactor[speed]

    onFactsChanged: {
        chartView.reset()
    }

    Connections {
        target: app.vehicles
        onCurrentDataReceived: chartView.appendData();
    }

    /*Timer {
        interval: 100
        repeat: true
        running: true
        onTriggered: chartView.appendData()
    }*/

    ChartView {
        id: chartView

        antialiasing: smooth
        legend.visible: false
        margins.top: 0
        margins.left: 0
        margins.bottom: 0
        margins.right: 0

        anchors.fill: parent
        onPlotAreaChanged: anchors.margins=-plotArea.y/3

        plotAreaColor: "black"
        backgroundColor: "black"
        backgroundRoundness: 0
        dropShadowEnabled: false

        property int samples: Math.min(1000,Math.max(25,width/(3*speedFactorValue)))
        property int time: 0

        ValueAxis {
            id: axisX
            property real t: chartView.time
            Behavior on t { enabled: app.settings.smooth.value && chartView.time>0; NumberAnimation {duration: 100; } }
            min: t-chartView.samples+20
            max: t
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
            tickCount: 2
            labelsColor: "white"
            labelsFont.pointSize: Qt.application.font.pixelSize * 0.7
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
            axisY.applyNiceNumbers()
            speed=0
        }

        function appendData()
        {
            time++;
            for(var i=0;i<facts.length;++i){
                if(i>=chartView.count)addFactSeries(facts[i])
                var s=chartView.series(i)
                var v=facts[i].value
                s.append(time,v);
                sdata.push(v)
                //instant rescale - grow
                if(axisY.max<v){
                    axisY.max=v+dataPadding;
                }
                if(axisY.min>v){
                    axisY.min=v-dataPadding;
                }
                //remove old
                if(s.count>samples) s.removePoints(0,s.count-samples)
            }
            //calc scale - reduce
            if((time-timeRescale)>21){
                timeRescale=time
                var d=sdata.length-samples*facts.length
                if(d>0)sdata.splice(0,d)
                var p=app.seriesBounds(sdata)
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
                if(bmod)axisY.applyNiceNumbers()
            }

        }

        function addFactSeries(fact)
        {
            var s = chartView.createSeries(ChartView.SeriesTypeLine,fact.title,axisX, axisY)
            s.useOpenGL = Qt.binding(function(){return openGL})
            s.capStyle=Qt.RoundCap
            //s.opacity=0.7
            if(fact.name.startsWith("cmd")){
                s.width=Qt.binding(function(){return lineWidthCmd})
                s.color=Qt.hsla(fact.color.hslHue, fact.color.hslSaturation/2, fact.color.hslLightness*1.2, 1)
            }else{
                s.width=Qt.binding(function(){return lineWidth})
                //s.color=Qt.hsla(fact.color.hslHue, fact.color.hslSaturation, fact.color.hslLightness, 0.5)
                s.color=fact.color
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

    MouseArea {
        anchors.fill: parent
        onDoubleClicked: changeSpeed()
    }

}


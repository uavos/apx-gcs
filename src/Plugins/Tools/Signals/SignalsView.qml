/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick
import QtCharts
import QtQuick.Controls.Material
import QtQml

Item {
    id: chartItem

    property var facts: []
    property var uiContext: ui
    property var apxContext: apx

    property bool openGL: false //apx.settings.graphics.opengl.value
    property bool smoothLines: uiContext ? uiContext.smooth : false

    property real speed: 0
    property real lineWidth: uiContext && uiContext.antialiasing ? 1.5 : 1
    property real lineWidthCmd: uiContext && uiContext.antialiasing ? 2.1 : 2

    property var speedFactor: [0.2, 0.5, 1, 2, 4]
    property real speedFactorValue: speed < 0
                                    ? speedFactor[0]
                                    : speed >= speedFactor.length
                                      ? speedFactor[speedFactor.length - 1]
                                      : speedFactor[speed]

    QtObject {
        id: colorProbe

        property color value: "white"
    }

    onFactsChanged: chartView.reset()

    Connections {
        target: chartItem.apxContext ? chartItem.apxContext.fleet.current.mandala : null

        function onTelemetryDecoded()
        {
            chartView.appendData()
        }
    }

    function normalizeBindText(value)
    {
        var text = String(value === undefined || value === null ? "" : value).trim()
        var simplePath = /^(?:mandala\.)?[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)+(?:\.value)?$/

        if (text === "" || !simplePath.test(text))
            return text

        if (text.startsWith("mandala."))
            text = text.slice(8)
        if (text.endsWith(".value"))
            text = text.slice(0, -6)

        return text
    }

    function itemBind(fact)
    {
        if (!fact)
            return ""

        if (typeof fact.bindText === "function")
            return normalizeBindText(fact.bindText())

        if (fact.bind !== undefined && fact.bind !== null && String(fact.bind) !== "")
            return normalizeBindText(fact.bind)
        if (fact.name !== undefined && fact.name !== null)
            return normalizeBindText(fact.name)

        return ""
    }

    function itemTitle(fact, index)
    {
        if (fact && fact.title !== undefined && String(fact.title) !== "")
            return String(fact.title)

        var bind = itemBind(fact)
        if (bind !== "")
            return bind

        return qsTr("Signal") + " " + (index + 1)
    }

    function defaultSeriesColor(index)
    {
        return Material.color(Material.Blue + index * 2)
    }

    function resolvedColor(value, fallback)
    {
        var source = value
        if (source === undefined || source === null || source === "")
            source = fallback

        colorProbe.value = source
        return colorProbe.value
    }

    function itemColor(fact, index)
    {
        if (fact) {
            if (typeof fact.colorValueCurrent === "function"
                    && String(fact.colorValueCurrent()) !== "")
                return resolvedColor(fact.colorValueCurrent(), defaultSeriesColor(index))

            if (fact.color !== undefined && fact.color !== null && String(fact.color) !== "")
                return resolvedColor(fact.color, defaultSeriesColor(index))

            if (fact.opts && fact.opts.color)
                return resolvedColor(fact.opts.color, defaultSeriesColor(index))
        }

        return resolvedColor("", defaultSeriesColor(index))
    }

    function itemSaveTarget(fact)
    {
        if (!fact)
            return ""

        if (typeof fact.saveTarget === "function")
            return fact.saveTarget()

        if (fact.save === undefined || fact.save === null || typeof fact.save === "function")
            return ""

        return String(fact.save).trim()
    }

    function evaluateValue(fact, bind)
    {
        if (fact && typeof fact.bindText !== "function" && fact.value !== undefined)
            return fact.value

        if (!bind)
            return NaN

        try {
            return eval(normalizeBindText(bind))
        } catch (error) {
            return NaN
        }
    }

    function filteredItemValue(fact, value)
    {
        if (fact && typeof fact.updateFilters === "function")
            return fact.updateFilters(value)

        return value
    }

    function writeSavedValue(fact, value)
    {
        var saveTarget = itemSaveTarget(fact)
        if (saveTarget === "" || !chartItem.apxContext)
            return

        var saveFact = chartItem.apxContext.fleet.current.mandala.fact(saveTarget, true)
        if (saveFact)
            saveFact.setRawValueLocal(value)
    }

    function isCommandBind(bind)
    {
        return bind.indexOf("cmd.") === 0 || bind.indexOf("cmd") === 0
    }

    function applySeriesStyle(series, fact, index)
    {
        if (!series || !fact)
            return

        var bind = itemBind(fact)
        var color = itemColor(fact, index)

        if (isCommandBind(bind)) {
            series.width = Qt.binding(function() {
                return lineWidthCmd
            })
            series.color = Qt.hsla(color.hslHue,
                                   color.hslSaturation / 2,
                                   color.hslLightness * 1.2,
                                   1)
        } else {
            series.width = Qt.binding(function() {
                return lineWidth
            })
            series.color = color
        }
    }

    function syncSeries(series, fact, index)
    {
        if (!series || !fact)
            return

        var title = itemTitle(fact, index)
        if (series.name !== title)
            series.name = title

        applySeriesStyle(series, fact, index)
    }

    function updateSeriesColor(index)
    {
        if (index < 0 || index >= chartItem.facts.length || index >= chartView.count)
            return

        syncSeries(chartView.series(index), chartItem.facts[index], index)
    }

    ChartView {
        id: chartView

        antialiasing: chartItem.uiContext && chartItem.uiContext.antialiasing
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

        plotAreaColor: "black"
        backgroundColor: "black"
        backgroundRoundness: 0
        dropShadowEnabled: false

        property int samples: Math.min(1000,
                                       Math.max(25, width / (3 * chartItem.speedFactorValue)))
        property int time: 0

        property bool dataExist: false

        ValueAxis {
            id: axisX

            property real t: chartView.time

            Behavior on t {
                enabled: chartItem.uiContext && chartItem.uiContext.smooth && chartView.dataExist

                NumberAnimation {
                    duration: 500
                }
            }

            min: t - chartView.samples + 20
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
            tickCount: 4
            labelsColor: "white"
            labelsFont.pixelSize: 8
            gridLineColor: "#555555"
        }

        property real dataPadding: 0.05
        property real dataPaddingZero: 0.05
        property var sdata: []
        property int timeRescale: 0

        function reset()
        {
            chartView.removeAllSeries()
            chartView.sdata = []
            chartView.time = 0
            chartView.dataExist = false
            chartView.timeRescale = 0
            axisY.min = -dataPaddingZero
            axisY.max = dataPaddingZero
            axisY.tickCount = 4
            axisY.applyNiceNumbers()
        }

        function appendData()
        {
            var t = time + 1

            for (var i = 0; i < chartItem.facts.length; ++i)
                appendDataValue(chartItem.facts[i], t, i)

            if ((t - timeRescale) > 21) {
                timeRescale = t
                var d = sdata.length - samples * chartItem.facts.length
                if (d > 0)
                    sdata.splice(0, d)
                var p = chartItem.apxContext ? chartItem.apxContext.seriesBounds(sdata)
                                             : Qt.point(0, 0)
                var min = p.x - dataPadding
                var max = p.y + dataPadding
                if (min === max) {
                    min -= dataPaddingZero
                    max += dataPaddingZero
                }
                var bmod = false
                if (axisY.min < min) {
                    axisY.min = min
                    bmod = true
                }
                if (axisY.max > max) {
                    axisY.max = max
                    bmod = true
                }
                if (bmod) {
                    axisY.tickCount = 4
                    axisY.applyNiceNumbers()
                }
            }
            time = t
            dataExist = true
        }

        function appendDataValue(fact, t, i)
        {
            if (i >= chartView.count)
                addFactSeries(fact, i)

            var series = chartView.series(i)
            chartItem.syncSeries(series, fact, i)

            var value = chartItem.evaluateValue(fact, chartItem.itemBind(fact))

            if (!isFinite(value))
                value = 0

            value = chartItem.filteredItemValue(fact, value)

            if (!isFinite(value))
                value = 0

            chartItem.writeSavedValue(fact, value)

            series.append(t, value)
            sdata.push(value)
            if (axisY.max < value)
                axisY.max = value + dataPadding
            if (axisY.min > value)
                axisY.min = value - dataPadding
            var cnt = samples
            if (series.count > cnt)
                series.removePoints(0, series.count - cnt)
        }

        function addFactSeries(fact, index)
        {
            var series = chartView.createSeries(chartItem.uiContext
                                                && chartItem.uiContext.antialiasing
                                                ? ChartView.SeriesTypeLine
                                                : ChartView.SeriesTypeLine,
                                                chartItem.itemTitle(fact, index),
                                                axisX,
                                                axisY)
            series.useOpenGL = Qt.binding(function() {
                return openGL
            })
            series.capStyle = Qt.RoundCap
            chartItem.applySeriesStyle(series, fact, index)
            return series
        }
    }

    function changeSpeed()
    {
        if ((speed + 1) < speedFactor.length)
            speed++
        else
            speed = 0
    }
}

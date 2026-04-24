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

    property var seriesState: []

    FilterRegistry {
        id: filterRegistry
    }

    QtObject {
        id: colorProbe

        property color value: "white"
    }

    onFactsChanged: chartView.reset()

    Component.onDestruction: destroySeriesState()

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

        if (fact.bind !== undefined && fact.bind !== null && String(fact.bind) !== "")
            return normalizeBindText(fact.bind)
        if (fact.name !== undefined && fact.name !== null)
            return normalizeBindText(fact.name)

        return ""
    }

    function itemTitle(fact, index)
    {
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
            if (fact.color !== undefined && fact.color !== null && String(fact.color) !== "")
                return resolvedColor(fact.color, defaultSeriesColor(index))

            if (fact.opts && fact.opts.color)
                return resolvedColor(fact.opts.color, defaultSeriesColor(index))
        }

        return resolvedColor("", defaultSeriesColor(index))
    }

    function itemSaveTarget(fact)
    {
        if (!fact || fact.save === undefined || fact.save === null)
            return ""

        return String(fact.save).trim()
    }

    function evaluateValue(fact, bind)
    {
        if (fact && fact.value !== undefined)
            return fact.value

        if (!bind)
            return NaN

        try {
            return eval(normalizeBindText(bind))
        } catch (error) {
            return NaN
        }
    }

    function destroySeriesState()
    {
        for (var i = 0; i < seriesState.length; ++i) {
            var state = seriesState[i]
            if (!state || !(state.filters instanceof Array))
                continue

            for (var j = 0; j < state.filters.length; ++j) {
                var filterObject = state.filters[j]
                if (filterObject)
                    filterObject.destroy()
            }
        }

        seriesState = []
    }

    function invokeFilterMethod(filterObject, methodName, argument)
    {
        if (!filterObject)
            return undefined

        var method = filterObject[methodName]
        if (!method)
            return undefined

        if (argument === undefined)
            return method.call(filterObject)

        return method.call(filterObject, argument)
    }

    function buildSeriesState(fact, index)
    {
        var bind = itemBind(fact)
        var filterDefs = filterRegistry.normalizeFilters(fact && fact.filters instanceof Array
                                                         ? fact.filters
                                                         : [])
        var filters = []

        for (var i = 0; i < filterDefs.length; ++i) {
            var filterData = filterDefs[i]
            var source = filterRegistry.componentSource(filterData.type)
            if (!source)
                continue

            var component = Qt.createComponent(source)
            if (!component || component.status !== Component.Ready) {
                if (component)
                    component.destroy()
                continue
            }

            var filterObject = component.createObject(chartItem)
            component.destroy()

            if (!filterObject)
                continue

            invokeFilterMethod(filterObject, "load", filterData)
            invokeFilterMethod(filterObject, "reset")
            filters.push(filterObject)
        }

        var saveTarget = itemSaveTarget(fact)

        return {
            "bind": bind,
            "title": itemTitle(fact, index),
            "color": itemColor(fact, index),
            "filters": filters,
            "saveTarget": saveTarget,
            "saveFact": saveTarget !== "" && chartItem.apxContext
                        ? chartItem.apxContext.fleet.current.mandala.fact(saveTarget, true)
                        : null
        }
    }

    function seriesStateAt(index)
    {
        while (seriesState.length <= index)
            seriesState.push(null)

        if (!seriesState[index])
            seriesState[index] = buildSeriesState(chartItem.facts[index], index)

        return seriesState[index]
    }

    function applyFilterChain(state, value)
    {
        var filtered = value

        if (!state || !(state.filters instanceof Array))
            return filtered

        for (var i = 0; i < state.filters.length; ++i) {
            var filterObject = state.filters[i]
            var stepFilter = filterObject ? filterObject["step"] : null
            if (!stepFilter)
                continue

            filtered = stepFilter.call(filterObject, filtered)
        }

        return filtered
    }

    function writeSavedValue(state, value)
    {
        if (!state || !state.saveFact)
            return

        state.saveFact.setRawValueLocal(value)
    }

    function isCommandBind(bind)
    {
        return bind.indexOf("cmd.") === 0 || bind.indexOf("cmd") === 0
    }

    function applySeriesStyle(series, state)
    {
        if (!series || !state)
            return

        var color = state.color

        if (isCommandBind(state.bind)) {
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

    function updateSeriesColor(index, color)
    {
        if (index < 0 || index >= chartItem.facts.length)
            return

        var state = seriesStateAt(index)
        if (!state)
            return

        state.color = resolvedColor(color, defaultSeriesColor(index))

        if (index < chartView.count)
            applySeriesStyle(chartView.series(index), state)
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
            chartItem.destroySeriesState()
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
            var state = chartItem.seriesStateAt(i)
            var value = chartItem.evaluateValue(fact, state.bind)

            if (!isFinite(value))
                value = 0

            value = chartItem.applyFilterChain(state, value)

            if (!isFinite(value))
                value = 0

            chartItem.writeSavedValue(state, value)

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
            var state = chartItem.seriesStateAt(index)
            var series = chartView.createSeries(chartItem.uiContext
                                                && chartItem.uiContext.antialiasing
                                                ? ChartView.SeriesTypeLine
                                                : ChartView.SeriesTypeLine,
                                                state.title,
                                                axisX,
                                                axisY)
            series.useOpenGL = Qt.binding(function() {
                return openGL
            })
            series.capStyle = Qt.RoundCap
            chartItem.applySeriesStyle(series, state)
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

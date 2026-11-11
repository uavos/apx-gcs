/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2026, Aliaksei Stratsilatau <sa@uavos.com>
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
import QtQuick.Controls
import QtQml

import Apx.Common

// 3D traffic scene on a Canvas (QtQuick3D is not a project dependency): ENU world
// around the unit, the sea level (AMSL 0) is the ground grid. The aircraft are drawn
// at their reported positions extrapolated by velocity between the ADS-B replies
// (smoothed, no jumps), with the velocity vector (60 s), an altitude stem down to the
// sea level and a label. The reachability volumes (Traffic::reachVolume) are drawn as
// a stack of time slice rings with a wave: on every reply the live slice sweeps the
// horizon in ~1 s and turns red from the moment the slices intersect (tauS); the
// pause button stops the wave and the slider scrubs the slice time by hand.
// Orbit camera: left drag rotates, wheel zooms, right (or shift) drag pans,
// double click resets. A click on an aircraft highlights it (as on the radar)
Item {
    id: view

    property var traffic
    property bool running: visible

    readonly property real fs: Style.fontSize*0.8

    // colors
    readonly property color colRed: "#FF4C00"
    readonly property color colYel: "#FFC800"
    readonly property color colOk: "#9AD7E8"
    readonly property color colOwn: "#808080"
    readonly property color colUnit: "#00FFFF"
    readonly property color colVel: "#2E9BFF"
    readonly property color colGrid: "#1F2A36"
    readonly property color colGrid2: "#16202A"
    readonly property color colText: "#CCCCCC"

    function levelColor(level, own) {
        return own ? colOwn : level===2 ? colRed : level===1 ? colYel : colOk
    }
    function ca(c, a) { return Qt.rgba(c.r, c.g, c.b, a) }

    // alert thresholds of the plugin (fallbacks keep the view alive without them)
    readonly property real horizonS: traffic && traffic.horizonS ? traffic.horizonS : 60
    readonly property real horizonRedS: traffic && traffic.horizonRedS ? traffic.horizonRedS : 30
    readonly property real sepRadiusM: traffic && traffic.sepRadiusM ? traffic.sepRadiusM : 500
    readonly property real sepAltM: traffic && traffic.sepAltM ? traffic.sepAltM : 150

    readonly property bool unitOrigin: traffic ? traffic.unitOrigin : false
    readonly property real unitAltM: traffic ? traffic.unitAltM : NaN
    readonly property var unit: apx.fleet.current
    readonly property real unitYaw: unitOrigin && unit && unit.mandala ? unit.mandala.est.att.yaw.value : 0

    // vertical exaggeration
    property real vScale: 2

    // orbit camera: yaw/pitch deg, distance m, target in world meters (y is real AMSL)
    property real camYaw: 30
    property real camPitch: 28
    property real camDist: 40000
    property var camTarget: ({ x: 0, y: 500, z: 0 })
    property bool autoRotate: true
    property bool fitted: false
    readonly property real distMin: 2000
    readonly property real distMax: 300000

    // time of the live reachability slice, s. While playing a wave sweeps the
    // volumes from 0 to the horizon in ~1 s (ease out) on every ADS-B reply; the
    // wave of an aircraft and the own one turn red from the moment the wave time
    // reaches its tauS (the slices intersect from there on). Paused: the slider
    // scrubs the slice time by hand
    property real sliceT: 0
    property bool playing: true
    property double waveStartMs: 0

    // smoothed display positions by hex and the reachability volumes cache
    property var smoothPos: ({})
    property var volumes: ({})
    property double lastFrameMs: 0

    // the aircraft of the model as plain objects (the canvas reads them every frame)
    Instantiator {
        id: targets
        model: view.traffic ? view.traffic.aircraft : null
        delegate: QtObject {
            readonly property string hex: model.hex
            readonly property string callsign: model.callsign
            readonly property string category: model.category
            readonly property bool own: model.own
            readonly property bool onGround: model.onGround
            readonly property real altM: model.altM
            readonly property real gsKt: model.gsKt
            readonly property real track: model.track
            readonly property bool trackValid: model.trackValid
            readonly property real vsMs: model.vsMs
            readonly property real distKm: model.distKm
            readonly property real brg: model.brg
            readonly property real tauS: model.tauS
            readonly property int level: model.level
            readonly property double seenMs: model.seenMs
        }
    }

    // reread the volumes on every reply: the own one always, the threats and the highlight
    Connections {
        target: view.traffic ? view.traffic : null
        function onTrafficUpdated() { view.updateVolumes() }
        function onHighlightedChanged() { view.updateVolumes() }
    }
    onRunningChanged: {
        if (!running)
            return
        updateVolumes()
        hintTimer.restart()
    }

    function updateVolumes()
    {
        if (!traffic || !running)
            return
        var v = {}
        v[""] = traffic.reachVolume("")
        for (var i = 0; i < targets.count; ++i) {
            var a = targets.objectAt(i)
            if (!a || a.own || a.onGround)
                continue
            if (a.level > 0 || a.hex === traffic.highlighted)
                v[a.hex] = traffic.reachVolume(a.hex)
        }
        volumes = v
        if (playing)
            waveStartMs = Date.now() // restart the wave on every reply
        if (!fitted)
            fitScene()
    }

    // initial camera fit: once, by the farthest aircraft
    function fitScene()
    {
        var far = 10000
        for (var i = 0; i < targets.count; ++i) {
            var a = targets.objectAt(i)
            if (a)
                far = Math.max(far, a.distKm*1000)
        }
        if (targets.count > 0)
            fitted = true
        camDist = Math.min(Math.max(far*1.7, distMin), distMax)
        camTarget = { x: 0, y: isNaN(unitAltM) ? 0 : unitAltM, z: 0 }
    }

    function resetCamera()
    {
        camYaw = 30
        camPitch = 28
        fitted = false
        fitScene()
        autoRotate = true
    }

    // ENU of an aircraft at its last reply, m (x east, z north, y AMSL)
    function replyPos(a)
    {
        var b = a.brg*Math.PI/180
        var d = a.distKm*1000
        return { x: d*Math.sin(b), y: isNaN(a.altM) ? 0 : a.altM, z: d*Math.cos(b) }
    }
    // extrapolated by the velocity since the reply (dead reckoning, clamped)
    function targetPos(a, nowMs)
    {
        var p = replyPos(a)
        var age = Math.min(Math.max((nowMs - a.seenMs)/1000, 0), 5)
        var v = a.gsKt*0.514444
        var t = a.track*Math.PI/180
        p.x += v*age*Math.sin(t)
        p.z += v*age*Math.cos(t)
        if (!isNaN(a.altM))
            p.y += a.vsMs*age
        return p
    }

    // frame: smooth the positions, advance the slice time, repaint
    Timer {
        interval: 33
        repeat: true
        running: view.running
        onTriggered: view.frame()
    }

    function frame()
    {
        var now = Date.now()
        var dt = lastFrameMs > 0 ? Math.min((now - lastFrameMs)/1000, 0.1) : 0.033
        lastFrameMs = now

        if (playing && waveStartMs > 0) {
            var wp = Math.min(Math.max((now - waveStartMs)/1000, 0), 1)
            sliceT = (1 - Math.pow(1 - wp, 3))*horizonS // ease out, done in ~1 s
        }
        if (!tSlider.pressed)
            tSlider.value = sliceT
        if (autoRotate)
            camYaw = (camYaw + dt*3) % 360

        // critically damped approach to the extrapolated positions
        var k = 1 - Math.exp(-dt/0.4)
        var seen = {}
        for (var i = 0; i < targets.count; ++i) {
            var a = targets.objectAt(i)
            if (!a)
                continue
            var p = targetPos(a, now)
            var s = smoothPos[a.hex]
            if (!s)
                smoothPos[a.hex] = s = { x: p.x, y: p.y, z: p.z }
            else {
                s.x += (p.x - s.x)*k
                s.y += (p.y - s.y)*k
                s.z += (p.z - s.z)*k
            }
            seen[a.hex] = true
        }
        for (var hex in smoothPos) {
            if (!seen[hex])
                delete smoothPos[hex]
        }
        canvas.requestPaint()
    }

    // camera basis for this frame
    function makeCamera()
    {
        var cy = camYaw*Math.PI/180
        var cp = Math.min(Math.max(camPitch, 5), 85)*Math.PI/180
        var ty = camTarget.y*vScale
        var eye = {
            x: camTarget.x + camDist*Math.cos(cp)*Math.sin(cy),
            y: ty + camDist*Math.sin(cp),
            z: camTarget.z + camDist*Math.cos(cp)*Math.cos(cy)
        }
        var f = norm3(camTarget.x - eye.x, ty - eye.y, camTarget.z - eye.z)
        var r = norm3(f.z, 0, -f.x) // cross(world up, f): screen right
        var u = { x: f.y*r.z - f.z*r.y, y: f.z*r.x - f.x*r.z, z: f.x*r.y - f.y*r.x } // cross(f, r)
        return {
            eye: eye, f: f, r: r, u: u,
            w: canvas.width, h: canvas.height,
            focal: (canvas.height/2)/Math.tan(22.5*Math.PI/180)
        }
    }
    function norm3(x, y, z)
    {
        var l = Math.sqrt(x*x + y*y + z*z)
        return l > 0 ? { x: x/l, y: y/l, z: z/l } : { x: 0, y: 0, z: 1 }
    }
    // world (x east, yReal AMSL m, z north) to camera space
    function toCam(cam, x, yReal, z)
    {
        var dx = x - cam.eye.x
        var dy = yReal*vScale - cam.eye.y
        var dz = z - cam.eye.z
        return {
            x: dx*cam.r.x + dy*cam.r.y + dz*cam.r.z,
            y: dx*cam.u.x + dy*cam.u.y + dz*cam.u.z,
            z: dx*cam.f.x + dy*cam.f.y + dz*cam.f.z
        }
    }
    function camToScreen(cam, c)
    {
        return { x: cam.w/2 + cam.focal*c.x/c.z, y: cam.h/2 - cam.focal*c.y/c.z, d: c.z }
    }
    // world to screen, null behind the camera
    function project(cam, x, yReal, z)
    {
        var c = toCam(cam, x, yReal, z)
        return c.z < 1 ? null : camToScreen(cam, c)
    }
    // a line segment clipped by the near plane, null when fully behind
    function projectLine(cam, x1, y1, z1, x2, y2, z2)
    {
        var a = toCam(cam, x1, y1, z1)
        var b = toCam(cam, x2, y2, z2)
        if (a.z < 1 && b.z < 1)
            return null
        if (a.z < 1)
            a = clipNear(a, b)
        else if (b.z < 1)
            b = clipNear(b, a)
        return [camToScreen(cam, a), camToScreen(cam, b)]
    }
    function clipNear(a, b)
    {
        var f = (1 - a.z)/(b.z - a.z)
        return { x: a.x + (b.x - a.x)*f, y: a.y + (b.y - a.y)*f, z: 1 }
    }

    // screen positions of the aircraft symbols of the last frame (click hit test)
    property var hits: []

    Canvas {
        id: canvas
        anchors.fill: parent

        readonly property string fam: apx.font_narrow(10).family

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.fillStyle = "#0B1016"
            ctx.fillRect(0, 0, width, height)
            if (!view.traffic)
                return
            view.render(ctx, view.makeCamera())
        }
    }

    function render(ctx, cam)
    {
        drawGrid(ctx, cam)

        var prims = []  // translucent world primitives, sorted far to near
        var labels = [] // screen space labels, drawn over everything
        var newHits = []
        var now = Date.now()

        // the unit (or the map center marker)
        var uAlt = isNaN(unitAltM) ? 0 : unitAltM
        var up = project(cam, 0, uAlt, 0)
        if (up) {
            addStem(prims, cam, 0, uAlt, 0, colUnit, up.d)
            var upt = up
            var yaw = unitYaw
            prims.push({ d: up.d, f: function(c) { drawUnit(c, cam, upt, yaw, uAlt) } })
        }

        // the wave of an aircraft turns red once the wave time reaches its tauS,
        // the own wave with the earliest tauS among the drawn volumes
        var minTau = Infinity
        for (var mi = 0; mi < targets.count; ++mi) {
            var ma = targets.objectAt(mi)
            if (ma && volumes[ma.hex] && isFinite(ma.tauS))
                minTau = Math.min(minTau, ma.tauS)
        }

        // own reachability volume
        addVolume(prims, cam, volumes[""], { x: 0, y: uAlt, z: 0 }, colUnit, true,
                  sliceT >= minTau ? colRed : colUnit)

        // the aircraft
        for (var i = 0; i < targets.count; ++i) {
            var a = targets.objectAt(i)
            if (!a)
                continue
            var s = smoothPos[a.hex]
            if (!s)
                continue
            var p = project(cam, s.x, s.y, s.z)
            if (!p)
                continue
            var col = levelColor(a.level, a.own)
            var noAlt = isNaN(a.altM)
            var hl = traffic.highlighted === a.hex

            if (!a.onGround)
                addStem(prims, cam, s.x, s.y, s.z, col, p.d)

            // velocity vector to the position in 60 s
            if (a.trackValid && !a.own && !a.onGround) {
                var v = a.gsKt*0.514444
                var tr = a.track*Math.PI/180
                var p2 = project(cam, s.x + v*horizonS*Math.sin(tr),
                                 noAlt ? s.y : s.y + a.vsMs*horizonS,
                                 s.z + v*horizonS*Math.cos(tr))
                if (p2) {
                    var ac = a.level > 0 ? col : colVel
                    prims.push({ d: p.d, f: makeArrow(p, p2, ac) })
                }
            }

            // the volume of a threat or of the highlighted aircraft
            if (volumes[a.hex])
                addVolume(prims, cam, volumes[a.hex], s, col, false,
                          isFinite(a.tauS) && sliceT >= a.tauS ? colRed : col)

            // the symbol: the screen angle of the track in this projection
            var ang = 0
            if (a.trackValid) {
                var tr2 = a.track*Math.PI/180
                var pd = project(cam, s.x + 500*Math.sin(tr2), s.y, s.z + 500*Math.cos(tr2))
                if (pd)
                    ang = Math.atan2(pd.y - p.y, pd.x - p.x) + Math.PI/2
            }
            prims.push({ d: p.d, f: makeSymbol(p, ang, a.category, col, a.onGround, noAlt, hl) })
            newHits.push({ x: p.x, y: p.y, hex: a.hex, own: a.own })

            labels.push({ d: p.d, hl: hl, f: makeLabel(p, a, col, noAlt) })
        }

        prims.sort(function(x, y) { return y.d - x.d })
        for (var j = 0; j < prims.length; ++j)
            prims[j].f(ctx)

        labels.sort(function(x, y) { return (x.hl ? 1 : 0) - (y.hl ? 1 : 0) || y.d - x.d })
        for (var l = 0; l < labels.length; ++l)
            labels[l].f(ctx)

        hits = newHits
    }

    // sea level grid around the camera target with range rings around the origin
    function drawGrid(ctx, cam)
    {
        var step = camDist > 150000 ? 20000 : camDist > 60000 ? 10000 : camDist > 25000 ? 5000 : 1000
        var ext = Math.min(Math.max(camDist*2, 4*step), 24*step)
        var cx0 = Math.round(camTarget.x/step)*step
        var cz0 = Math.round(camTarget.z/step)*step
        ctx.lineWidth = 1
        ctx.setLineDash([])
        var n = Math.floor(ext/step)
        for (var i = -n; i <= n; ++i) {
            var off = i*step
            var fade = Math.max(0, 1 - Math.pow(Math.abs(off)/ext, 1.5))
            // along z (north-south) at x = cx0+off
            gridLine(ctx, cam, cx0 + off, cz0 - ext, cx0 + off, cz0 + ext,
                     ca((cx0 + off) % (5*step) === 0 ? colGrid : colGrid2, 0.9*fade))
            // along x (east-west) at z = cz0+off
            gridLine(ctx, cam, cx0 - ext, cz0 + off, cx0 + ext, cz0 + off,
                     ca((cz0 + off) % (5*step) === 0 ? colGrid : colGrid2, 0.9*fade))
        }
        // range rings around the origin, with labels to the east
        ctx.font = Math.round(fs*0.9) + "px \"" + canvas.fam + "\""
        ctx.textAlign = "left"
        ctx.textBaseline = "middle"
        var rings = [10000, 20000, 50000, 100000]
        for (var r = 0; r < rings.length; ++r) {
            var rad = rings[r]
            if (rad > ext*2.5)
                continue
            ctx.strokeStyle = ca(colText, 0.14)
            ctx.beginPath()
            var started = false
            for (var k = 0; k <= 72; ++k) {
                var a = k/72*Math.PI*2
                var p = project(cam, rad*Math.sin(a), 0, rad*Math.cos(a))
                if (!p) { started = false; continue }
                if (!started) { ctx.moveTo(p.x, p.y); started = true }
                else ctx.lineTo(p.x, p.y)
            }
            ctx.stroke()
            var lp = project(cam, rad, 0, 0)
            if (lp) {
                ctx.fillStyle = ca(colText, 0.4)
                ctx.fillText((rad/1000) + " km", lp.x + 4, lp.y)
            }
        }
    }
    function gridLine(ctx, cam, x1, z1, x2, z2, style)
    {
        var s = projectLine(cam, x1, 0, z1, x2, 0, z2)
        if (!s)
            return
        ctx.strokeStyle = style
        ctx.beginPath()
        ctx.moveTo(s[0].x, s[0].y)
        ctx.lineTo(s[1].x, s[1].y)
        ctx.stroke()
    }

    // dashed altitude stem down to the sea level with a ground dot
    function addStem(prims, cam, x, y, z, col, depth)
    {
        var top = project(cam, x, y, z)
        var bot = project(cam, x, 0, z)
        if (!top || !bot)
            return
        var c = ca(col, 0.35)
        prims.push({ d: depth + 1, f: function(ctx) {
            ctx.strokeStyle = c
            ctx.lineWidth = 1
            ctx.setLineDash([4, 4])
            ctx.beginPath()
            ctx.moveTo(top.x, top.y)
            ctx.lineTo(bot.x, bot.y)
            ctx.stroke()
            ctx.setLineDash([])
            ctx.fillStyle = c
            ctx.beginPath()
            ctx.arc(bot.x, bot.y, 2, 0, Math.PI*2)
            ctx.fill()
        } })
    }

    // reachability volume: the slice rings lofted by the corner edges and the live
    // (wave) slice, colored liveCol from the moment the slices intersect
    function addVolume(prims, cam, vol, base, col, own, liveCol)
    {
        if (!vol || vol.length === 0)
            return
        var fillA = own ? 0.04 : 0.05
        var corners = [] // 4 corner points per ring, world
        for (var i = 0; i < vol.length; ++i) {
            var s = vol[i]
            var pts = ringWorld(s, base)
            if (pts.length < 3)
                continue
            var bright = 0.12 + 0.25*i/Math.max(vol.length - 1, 1)
            addRing(prims, cam, pts, ca(col, bright), ca(col, fillA), 1)
            var n = pts.length
            corners.push([pts[0], pts[Math.floor(n/4)], pts[Math.floor(n/2)], pts[Math.floor(3*n/4)]])
        }
        // the edges between the consecutive rings
        var ec = ca(col, 0.22)
        for (var j = 0; j + 1 < corners.length; ++j) {
            for (var k = 0; k < 4; ++k) {
                let a = project(cam, corners[j][k].x, corners[j][k].y, corners[j][k].z)
                let b = project(cam, corners[j + 1][k].x, corners[j + 1][k].y, corners[j + 1][k].z)
                if (!a || !b)
                    continue
                prims.push({ d: (a.d + b.d)/2, f: function(ctx) {
                    ctx.strokeStyle = ec
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    ctx.moveTo(a.x, a.y)
                    ctx.lineTo(b.x, b.y)
                    ctx.stroke()
                } })
            }
        }
        // the live (wave) slice, brighter and filled
        var live = sliceAt(vol, sliceT)
        if (live) {
            var lc = liveCol === undefined ? col : liveCol
            var lp = ringWorld(live, base)
            if (lp.length >= 3)
                addRing(prims, cam, lp, ca(lc, 0.95), ca(lc, own ? 0.1 : 0.14), 1.5)
        }
    }
    // ring of a slice in world coordinates: relative to the aircraft, at the band center
    function ringWorld(s, base)
    {
        var y = base.y + (s.lo + s.hi)/2
        var pts = []
        for (var i = 0; i + 1 < s.ring.length; i += 2)
            pts.push({ x: base.x + s.ring[i], y: y, z: base.z + s.ring[i + 1] })
        return pts
    }
    function addRing(prims, cam, pts, stroke, fill, lw)
    {
        var sp = []
        var d = 0
        for (var i = 0; i < pts.length; ++i) {
            var p = project(cam, pts[i].x, pts[i].y, pts[i].z)
            if (!p)
                return
            sp.push(p)
            d += p.d
        }
        d /= pts.length
        prims.push({ d: d, f: function(ctx) {
            ctx.beginPath()
            ctx.moveTo(sp[0].x, sp[0].y)
            for (var k = 1; k < sp.length; ++k)
                ctx.lineTo(sp[k].x, sp[k].y)
            ctx.closePath()
            ctx.fillStyle = fill
            ctx.fill()
            ctx.strokeStyle = stroke
            ctx.lineWidth = lw
            ctx.stroke()
        } })
    }
    // the live slice interpolated between the sampled rings
    function sliceAt(vol, t)
    {
        if (!vol || vol.length === 0)
            return null
        if (t <= vol[0].t)
            return vol[0]
        for (var i = 0; i + 1 < vol.length; ++i) {
            var a = vol[i], b = vol[i + 1]
            if (t > b.t)
                continue
            if (a.ring.length !== b.ring.length)
                return b
            var f = (t - a.t)/(b.t - a.t)
            var ring = new Array(a.ring.length)
            for (var k = 0; k < ring.length; ++k)
                ring[k] = a.ring[k] + (b.ring[k] - a.ring[k])*f
            return { t: t, ring: ring, lo: a.lo + (b.lo - a.lo)*f, hi: a.hi + (b.hi - a.hi)*f }
        }
        return vol[vol.length - 1]
    }

    // velocity vector with a small head
    function makeArrow(p1, p2, col)
    {
        var c = ca(col, 0.7)
        return function(ctx) {
            ctx.strokeStyle = c
            ctx.lineWidth = 1
            ctx.setLineDash([])
            ctx.beginPath()
            ctx.moveTo(p1.x, p1.y)
            ctx.lineTo(p2.x, p2.y)
            var a = Math.atan2(p2.y - p1.y, p2.x - p1.x)
            var hl = 6
            ctx.moveTo(p2.x, p2.y)
            ctx.lineTo(p2.x - hl*Math.cos(a - 0.5), p2.y - hl*Math.sin(a - 0.5))
            ctx.moveTo(p2.x, p2.y)
            ctx.lineTo(p2.x - hl*Math.cos(a + 0.5), p2.y - hl*Math.sin(a + 0.5))
            ctx.stroke()
        }
    }

    // aircraft silhouette (half, nose up, mirrored when drawn)
    readonly property var planeHalf: [[0, -1.1], [0.16, -0.6], [0.2, -0.15], [1.05, 0.14],
                                      [1.05, 0.32], [0.22, 0.3], [0.28, 0.8], [0.62, 1.0],
                                      [0.62, 1.14], [0.1, 1.05], [0, 1.05]]

    function makeSymbol(p, ang, category, col, onGround, noAlt, hl)
    {
        var size = Math.min(Math.max(14*8000/p.d, 8), 20)
        var alpha = noAlt ? 0.55 : 1
        var heli = category === "A7"
        var dot = category.length > 0 && (category[0] === "B" || category[0] === "C") && category !== "B1" && category !== "B4"
        var half = planeHalf
        return function(ctx) {
            ctx.save()
            ctx.translate(p.x, p.y)
            ctx.globalAlpha = alpha
            if (onGround) {
                // flat ground marker
                ctx.fillStyle = ca(col, 0.5)
                ctx.fillRect(-size*0.25, -size*0.25, size*0.5, size*0.5)
            } else if (heli) {
                ctx.strokeStyle = col
                ctx.fillStyle = ca(col, 0.25)
                ctx.lineWidth = 1.5
                ctx.beginPath()
                ctx.arc(0, 0, size*0.4, 0, Math.PI*2)
                ctx.fill()
                ctx.stroke()
                ctx.beginPath()
                for (var r = 0; r < 4; ++r) {
                    var ra = ang + Math.PI/4 + r*Math.PI/2
                    ctx.moveTo(0, 0)
                    ctx.lineTo(size*0.6*Math.cos(ra), size*0.6*Math.sin(ra))
                }
                ctx.stroke()
            } else if (dot) {
                ctx.rotate(ang)
                ctx.fillStyle = col
                ctx.beginPath()
                ctx.moveTo(0, -size*0.45)
                ctx.lineTo(size*0.32, 0)
                ctx.lineTo(0, size*0.45)
                ctx.lineTo(-size*0.32, 0)
                ctx.closePath()
                ctx.fill()
            } else {
                ctx.rotate(ang)
                ctx.fillStyle = col
                ctx.beginPath()
                var s = size*0.5
                ctx.moveTo(half[0][0]*s, half[0][1]*s)
                for (var i = 1; i < half.length; ++i)
                    ctx.lineTo(half[i][0]*s, half[i][1]*s)
                for (var j = half.length - 1; j >= 0; --j)
                    ctx.lineTo(-half[j][0]*s, half[j][1]*s)
                ctx.closePath()
                ctx.fill()
            }
            ctx.restore()
            if (hl) {
                ctx.strokeStyle = "#FFFFFF"
                ctx.lineWidth = 1
                ctx.strokeRect(p.x - size*0.7, p.y - size*0.7, size*1.4, size*1.4)
            }
            ctx.globalAlpha = 1
        }
    }

    // the unit: a thin airplane cross of lines with a soft glow
    function drawUnit(ctx, cam, p, yawDeg, alt)
    {
        var s = Math.min(Math.max(16*8000/p.d, 10), 24)
        // glow
        var g = ctx.createRadialGradient(p.x, p.y, 0, p.x, p.y, s*1.6)
        g.addColorStop(0, ca(colUnit, 0.22))
        g.addColorStop(1, ca(colUnit, 0))
        ctx.fillStyle = g
        ctx.beginPath()
        ctx.arc(p.x, p.y, s*1.6, 0, Math.PI*2)
        ctx.fill()

        ctx.save()
        ctx.translate(p.x, p.y)
        ctx.strokeStyle = colUnit
        ctx.lineWidth = 2
        if (unitOrigin) {
            // screen angle of the unit yaw in this projection
            var yr = yawDeg*Math.PI/180
            var pd = project(cam, 500*Math.sin(yr), alt, 500*Math.cos(yr))
            var ang = 0
            if (pd)
                ang = Math.atan2(pd.y - p.y, pd.x - p.x) + Math.PI/2
            ctx.rotate(ang)
            ctx.beginPath()
            ctx.moveTo(0, -s*0.7)
            ctx.lineTo(0, s*0.5)
            ctx.moveTo(-s*0.55, -s*0.05)
            ctx.lineTo(s*0.55, -s*0.05)
            ctx.moveTo(-s*0.25, s*0.42)
            ctx.lineTo(s*0.25, s*0.42)
            ctx.stroke()
        } else {
            // map center origin: a crosshair
            ctx.beginPath()
            ctx.arc(0, 0, s*0.4, 0, Math.PI*2)
            ctx.moveTo(-s*0.7, 0); ctx.lineTo(-s*0.2, 0)
            ctx.moveTo(s*0.2, 0); ctx.lineTo(s*0.7, 0)
            ctx.moveTo(0, -s*0.7); ctx.lineTo(0, -s*0.2)
            ctx.moveTo(0, s*0.2); ctx.lineTo(0, s*0.7)
            ctx.stroke()
        }
        ctx.restore()
    }

    // two line label with a rounded dark plate; the climb/descent triangle is drawn
    // by hand (the narrow font has no arrow glyphs)
    function makeLabel(p, a, col, noAlt)
    {
        var line1 = a.callsign.length > 0 ? a.callsign : a.hex.toUpperCase()
        var line2 = a.onGround ? "GND" : noAlt ? "---" : Math.round(a.altM) + " m"
        var vs = (!a.onGround && Math.abs(a.vsMs) > 0.5) ? (a.vsMs > 0 ? 1 : -1) : 0
        if (isFinite(a.tauS))
            line2 += "  τ " + Math.round(a.tauS) + " s"
        var px = Math.round(fs)
        var fam = canvas.fam
        return function(ctx) {
            ctx.font = "bold " + px + "px \"" + fam + "\""
            var w1 = ctx.measureText(line1).width
            ctx.font = px + "px \"" + fam + "\""
            var w2 = ctx.measureText(line2).width + (vs !== 0 ? px : 0)
            var w = Math.max(w1, w2) + px
            var h = px*2.6
            var x = p.x + 10
            var y = p.y - h - 6
            ctx.fillStyle = "rgba(8,12,18,0.75)"
            roundRect(ctx, x, y, w, h, 3)
            ctx.fill()
            ctx.textAlign = "left"
            ctx.textBaseline = "middle"
            ctx.font = "bold " + px + "px \"" + fam + "\""
            ctx.fillStyle = col
            ctx.fillText(line1, x + px*0.5, y + h*0.28)
            ctx.font = px + "px \"" + fam + "\""
            ctx.fillStyle = "#DDDDDD"
            ctx.fillText(line2, x + px*0.5, y + h*0.74)
            if (vs !== 0) {
                // climb or descent triangle after the text
                var tx = x + px*0.5 + ctx.measureText(line2).width + px*0.45
                var ty = y + h*0.74
                ctx.fillStyle = col
                ctx.beginPath()
                ctx.moveTo(tx, ty - vs*px*0.3)
                ctx.lineTo(tx - px*0.25, ty + vs*px*0.2)
                ctx.lineTo(tx + px*0.25, ty + vs*px*0.2)
                ctx.closePath()
                ctx.fill()
            }
        }
    }
    function roundRect(ctx, x, y, w, h, r)
    {
        ctx.beginPath()
        ctx.moveTo(x + r, y)
        ctx.lineTo(x + w - r, y)
        ctx.quadraticCurveTo(x + w, y, x + w, y + r)
        ctx.lineTo(x + w, y + h - r)
        ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
        ctx.lineTo(x + r, y + h)
        ctx.quadraticCurveTo(x, y + h, x, y + h - r)
        ctx.lineTo(x, y + r)
        ctx.quadraticCurveTo(x, y, x + r, y)
        ctx.closePath()
    }

    // orbit, zoom, pan and the aircraft click
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        property real lastX: 0
        property real lastY: 0
        property bool dragging: false
        property real pressX: 0
        property real pressY: 0

        onPressed: (mouse) => {
            lastX = mouse.x
            lastY = mouse.y
            pressX = mouse.x
            pressY = mouse.y
            dragging = false
            view.autoRotate = false
        }
        onPositionChanged: (mouse) => {
            if (!pressed)
                return
            var dx = mouse.x - lastX
            var dy = mouse.y - lastY
            lastX = mouse.x
            lastY = mouse.y
            if (!dragging && Math.hypot(mouse.x - pressX, mouse.y - pressY) > 4)
                dragging = true
            if (!dragging)
                return
            if (mouse.buttons & Qt.RightButton || mouse.modifiers & Qt.ShiftModifier) {
                // pan the target in the horizontal plane (the world follows the mouse)
                var wpp = view.camDist/((height/2)/Math.tan(22.5*Math.PI/180))
                var cy = view.camYaw*Math.PI/180
                var t = view.camTarget
                t.x += (dx*Math.cos(cy) - dy*Math.sin(cy))*wpp
                t.z += (-dx*Math.sin(cy) - dy*Math.cos(cy))*wpp
                view.camTarget = t
            } else {
                view.camYaw = (view.camYaw - dx*0.4) % 360
                view.camPitch = Math.min(Math.max(view.camPitch + dy*0.3, 5), 85)
            }
        }
        onReleased: (mouse) => {
            if (dragging || mouse.button !== Qt.LeftButton || !view.traffic)
                return
            // click: toggle the highlight of the nearest aircraft symbol
            var best = null
            var bd = 16
            for (var i = 0; i < view.hits.length; ++i) {
                var h = view.hits[i]
                var d = Math.hypot(mouse.x - h.x, mouse.y - h.y)
                if (d < bd) {
                    bd = d
                    best = h
                }
            }
            if (!best)
                return
            view.traffic.selected = ""
            view.traffic.highlighted = (view.traffic.highlighted === best.hex || best.own) ? "" : best.hex
        }
        onDoubleClicked: view.resetCamera()
        onWheel: (wheel) => {
            view.autoRotate = false
            view.camDist = Math.min(Math.max(view.camDist*Math.pow(1.0015, -wheel.angleDelta.y),
                                             view.distMin), view.distMax)
        }
    }

    // overlay: header
    Column {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: Style.spacing*2
        spacing: 2
        Text {
            text: qsTr("TRAFFIC 3D")
            color: "#FFFFFF"
            font: apx.font_narrow(fs*1.4, true)
        }
        Text {
            text: targets.count + " " + qsTr("aircraft")
                  + (view.traffic && view.traffic.unitAltSrc
                     ? "  " + view.traffic.unitAltSrc + " " + Math.round(view.unitAltM) + " m" : "")
            color: "#AAAAAA"
            font: apx.font_narrow(fs)
        }
        Text {
            visible: view.traffic ? view.traffic.status.length > 0 : false
            text: view.traffic ? view.traffic.status : ""
            color: view.colRed
            font: apx.font_narrow(fs)
        }
    }

    // overlay: NO UNIT badge (the volumes and the alerts need the unit position)
    Text {
        visible: !view.unitOrigin && !(view.traffic && view.traffic.offline)
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.margins: Style.spacing*2
        text: qsTr("NO UNIT")
        color: view.colYel
        font: apx.font_narrow(fs*1.4, true)
    }

    // overlay: offline plate (the scene keeps drawing the last aircraft)
    Row {
        visible: view.traffic ? view.traffic.offline : false
        anchors.centerIn: parent
        spacing: Style.spacing*2
        MaterialIcon {
            anchors.verticalCenter: parent.verticalCenter
            name: "wifi-off"
            size: fs*3
            color: view.colYel
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("OFFLINE")
            color: view.colYel
            font: apx.font_narrow(fs*2, true)
        }
    }

    // overlay: vertical scale and the mouse hint
    Column {
        id: rightPanel
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Style.spacing*2
        spacing: Style.spacing
        Row {
            anchors.right: parent.right
            spacing: Style.spacing
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Alt scale")
                color: "#888888"
                font: apx.font_narrow(fs)
            }
            Repeater {
                model: [1, 2, 5]
                delegate: TextButton {
                    size: Style.buttonSize*0.7
                    text: "×" + modelData
                    checkable: true
                    checked: view.vScale === modelData
                    textColor: checked ? "#FFC800" : "#CCCCCC"
                    onTriggered: view.vScale = modelData
                }
            }
        }
        Text {
            anchors.right: parent.right
            text: qsTr("drag orbit · wheel zoom · right drag pan · double click reset")
            color: "#777777"
            font: apx.font_narrow(fs*0.9)
            opacity: hintTimer.running ? 0.9 : 0
            Behavior on opacity { NumberAnimation { duration: 800 } }
        }
    }
    Timer {
        id: hintTimer
        interval: 6000
    }

    // overlay: legend
    Column {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: Style.spacing*2
        spacing: 2
        Repeater {
            model: [
                { c: view.colRed, t: qsTr("conflict within") + " " + view.horizonRedS + " s" },
                { c: view.colYel, t: qsTr("conflict within") + " " + view.horizonS + " s" },
                { c: view.colOk, t: qsTr("clear") }
            ]
            delegate: Row {
                spacing: Style.spacing
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: fs*0.8
                    height: fs*0.8
                    radius: 2
                    color: modelData.c
                }
                Text {
                    text: modelData.t
                    color: "#AAAAAA"
                    font: apx.font_narrow(fs)
                }
            }
        }
        Text {
            text: qsTr("separation") + " " + view.sepRadiusM + " m / ±" + view.sepAltM + " m"
            color: "#777777"
            font: apx.font_narrow(fs*0.9)
        }
    }

    // overlay: the time of the live reachability slice
    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.margins: Style.spacing*2
        spacing: Style.spacing*2

        MaterialIcon {
            anchors.verticalCenter: parent.verticalCenter
            name: view.playing ? "pause" : "play"
            size: fs*1.8
            color: "#CCCCCC"
            MouseArea {
                anchors.fill: parent
                anchors.margins: -Style.spacing
                onClicked: {
                    view.playing = !view.playing
                    if (view.playing)
                        view.waveStartMs = Date.now() // resume with a fresh wave
                }
            }
        }
        Slider {
            id: tSlider
            anchors.verticalCenter: parent.verticalCenter
            width: fs*20
            from: 0
            to: view.horizonS
            onMoved: {
                view.playing = false
                view.sliceT = value
            }
            background: Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: tSlider.width
                height: 3
                radius: 1.5
                color: "#2A3542"
                Rectangle {
                    width: tSlider.visualPosition*parent.width
                    height: parent.height
                    radius: 1.5
                    color: "#5A7A94"
                }
            }
            handle: Rectangle {
                x: tSlider.visualPosition*(tSlider.width - width)
                anchors.verticalCenter: parent.verticalCenter
                width: fs*0.9
                height: fs*0.9
                radius: width/2
                color: tSlider.pressed ? "#FFFFFF" : "#CCCCCC"
            }
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "t = " + Math.round(view.sliceT) + " s"
            color: "#CCCCCC"
            font: apx.font_narrow(fs, true)
        }
    }
}

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

Image {
    property string elementName
    property string svgFileName
    property int vSlice: 0
    property int vSliceCount: 0
    property int hSlice: 0
    property int hSliceCount: 0
    //border property is useful to extent the area of image e bit,
    //so it looks well antialiased when rotated
    property int border: 0

    property string svgFileNamePath: {
        var s=svgFileName.trim()
        //console.log(s)
        var i=s.indexOf(":/")
        if(i>=0)s=s.slice(i+1)
        //console.log(" - "+s)
        return s
    }

    property rect elementBounds: svgRenderer.elementBounds(svgFileNamePath, elementName)

    property real scaleFactor: width/elementBounds.width

    sourceSize: Qt.size(Math.round(elementBounds.width*sourceScale),Math.round(elementBounds.height*sourceScale))
    property real sourceScale: 1

    Component.onCompleted: reloadImage()
    onElementNameChanged: reloadImage()
    onSourceSizeChanged: reloadImage()

    function reloadImage() {
        if(svgFileNamePath.trim().length<=0)
            return
        if(!svgRenderer)
            return

        var src = "image://svg/"+svgFileNamePath


        var params = []
        if(elementName != "")
            params.push("e="+elementName)
        if (hSliceCount > 1)
            params.push("hslice="+hSlice+":"+hSliceCount)
        if (vSliceCount > 1)
            params.push("vslice="+vSlice+":"+vSliceCount)
        if (border > 0)
            params.push("border="+border)

        if (params.length>0)
            src += "?" + params.join('&')

        source=src
        elementBounds = svgRenderer.elementBounds(svgFileNamePath, elementName)
    }
}

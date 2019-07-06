import QtQuick 2.2

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

    property variant elementBounds: svgRenderer.elementBounds(svgFileNamePath, elementName)

    property double scaleFactor: width/elementBounds.width

    sourceSize.width: Math.round(width)
    sourceSize.height: Math.round(height)



    Component.onCompleted: reloadImage()
    onElementNameChanged: reloadImage()
    onSourceSizeChanged: reloadImage()

    function reloadImage() {
        if(svgFileNamePath.trim().length<=0)return

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

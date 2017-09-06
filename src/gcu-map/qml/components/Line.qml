import QtQuick 2.2

Rectangle {
    id: line
    property alias fromX: line.x
    property alias fromY: line.y

    property real toX: line.x
    property real toY: line.y

    color: "black"
    height: 2
    smooth: true;

    transformOrigin: Item.TopLeft;

    width: getWidth(fromX,fromY,toX,toY);
    rotation: getSlope(fromX,fromY,toX,toY);

    function getWidth(sx1,sy1,sx2,sy2)
    {
        var w = Math.sqrt(Math.pow((sx2-sx1),2)+Math.pow((sy2-sy1),2));
        return w;
    }

    function getSlope(sx1,sy1,sx2,sy2)
    {
        var a,m,d;
        var b = sx2-sx1;
        if (b === 0) {
            return 0;
        }
        a = sy2 - sy1;
        m = a/b;
        d = Math.atan(m) * 180 / Math.PI;

        if (a < 0 && b < 0) {
            return d + 180;
        }
        else if (a >= 0 && b >= 0) {
            return d;
        }
        else if (a < 0 && b >= 0) {
            return d;
        }
        else if (a >= 0 && b < 0) {
            return d + 180;
        }
        else {
            return 0;
        }
    }
}

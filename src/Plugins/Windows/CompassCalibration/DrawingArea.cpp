#include "DrawingArea.h"
#include <App/AppRoot.h>
#include <Fact/Fact.h>

DrawingArea::DrawingArea()
    : QWidget()
{
    pixMap = new QPixmap();
    painter = new QPainter;
    sAxis[0] = "";
    sAxis[1] = "";
    isTraced = false;
    cVector.setX(1.0);
    cVector.setY(1.0);
    xMin = 1;
    xMax = -1;
    yMin = 1;
    yMax = -1;
    //resizeArea(100, 100);
    //resize(100,100);
    setMinimumSize(160, 160);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}
void DrawingArea::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    resizeArea(event->size().width(), event->size().height());
}

DrawingArea::~DrawingArea() {}
QPointF DrawingArea::DPoint(double x, double y)
{
    return QPointF((x * dScale + dOffset), (-y * dScale + dOffset));
}
QPointF DrawingArea::DPoint(QPointF pt)
{
    return QPointF((pt.x() * dScale + dOffset), (-pt.y() * dScale + dOffset));
}
void DrawingArea::startTrace()
{
    isTraced = true;
}
void DrawingArea::stopTrace()
{
    isTraced = false;
}

void DrawingArea::clearTrace()
{
    cVectorList.clear();
    xMin = 1;
    xMax = -1;
    yMin = 1;
    yMax = -1;
    this->update();
}
void DrawingArea::addData(double x, double y)
{
    cVector.setX(x);
    cVector.setY(-y);
    if (isTraced) {
        xMin = (x < xMin) ? x : xMin;
        yMin = (y < yMin) ? y : yMin;
        xMax = (x > xMax) ? x : xMax;
        yMax = (y > yMax) ? y : yMax;
        cVectorList.append(cVector);
    }
    this->update();
}
void DrawingArea::resizeArea(int width, int height)
{
    int space = 25;
    dScale = (((width < height) ? width : height) - space * 2) / 2;

    dOffset = ((width < height) ? width : height) / 2;

    if ((width < 50) || (height < 50))
        return;
    //this->setFixedSize(width, height);
    //resize(width,height);

    delete pixMap;
    pixMap = new QPixmap(this->size());
    pixMap->fill(Qt::black);

    painter->begin(pixMap);

    QPen wpen(Qt::white, 1, Qt::SolidLine);
    QPen dgpen(QColor(20, 20, 50), 1, Qt::SolidLine);
    painter->setPen(dgpen);

    for (int i = -5; i <= 5; i++) {
        painter->drawLine(DPoint(i / 5.0, -1), DPoint(i / 5.0, 1));
    }

    for (int i = -5; i <= 5; i++) {
        painter->drawLine(DPoint(1, i / 5.0), DPoint(-1, i / 5.0));
    }

    painter->setPen(wpen);
    painter->drawLine(DPoint(0, -1), DPoint(0, 1));
    painter->drawLine(DPoint(-1, 0), DPoint(1, 0));

    for (int i = -10; i <= 10; i++) {
        painter->drawLine(DPoint(i / 10.0, -0.02), DPoint(i / 10.0, 0.02));
    }

    for (int i = -10; i <= 10; i++) {
        painter->drawLine(DPoint(0.02, i / 10.0), DPoint(-0.02, i / 10.0));
    }
    painter->drawText(DPoint(1.02, 0), sAxis[0]);
    painter->drawText(DPoint(0, 1.02), sAxis[1]);

    painter->end();
}

QPolygonF DrawingArea::getPolygon(int k)
{
    QPolygonF polygon;
    QPointF point(10, 10);

    QPen pen;
    pen.setCapStyle(Qt::RoundCap);
    pen.setColor(Qt::gray);
    pen.setWidth(1);
    painter->setPen(pen);

    QPointF poi(10, 10);
    poi = QPointF(poi.x() * k, poi.y() * k);
    poi += QPointF(this->width() / 2, this->height() / 2);

    painter->drawLine(poi, QPointF(this->width() / 2, poi.y()));
    painter->drawLine(poi, QPointF(poi.x(), this->height() / 2));

    painter->setPen(QPen());

    pen.setColor(Qt::red);
    pen.setWidth(4);
    painter->setPen(pen);
    painter->drawPoint(poi);
    painter->setPen(QPen());

    polygon.append(point);

    return polygon;
}
void DrawingArea::DrawData()
{
    double MaxRadius = 0;

    // Calculate Data
    double dx = (xMin + xMax) / 2;
    double dy = (yMin + yMax) / 2;

    double dxR = (xMax - xMin);
    double dyR = (yMax - yMin);

    double kxR = dxR; //(dxR <= dyR) ? dxR / dyR : 1;
    double kyR = dyR; //(dyR <= dxR) ? dyR / dxR : 1;

    double bearingRadians = std::atan2(cVector.y(), cVector.x()); // get bearing in radians
    double bearingDegrees = bearingRadians * (180.0 / M_PI);      // convert to degrees
    bearingDegrees = (bearingDegrees > 0.0 ? bearingDegrees
                                           : (360.0 + bearingDegrees)); // correct discontinuity

    bearingRadians = std::atan2(cVector.y() + dy, cVector.x() - dx); // get bearing in radians
    double bearingDegrees2 = bearingRadians * (180.0 / M_PI);        // convert to degrees
    bearingDegrees2 = (bearingDegrees2 > 0.0 ? bearingDegrees2
                                             : (360.0 + bearingDegrees2)); // correct discontinuity
    double angError = bearingDegrees - bearingDegrees2;

    bX = dx;
    bY = dy;
    sX = 1.0 / kxR;
    sY = 1.0 / kyR;

    QPen pen;
    pen.setCapStyle(Qt::RoundCap);

    // Draw Points
    pen.setColor(Qt::green);
    pen.setWidth(1);
    painter->setPen(pen);
    foreach (QPointF cv, cVectorList) {
        painter->drawPoint(DPoint(cv));
        double tx = cv.x() - dx;
        double ty = cv.y() + dy;
        double r = std::sqrt(tx * tx + ty * ty);
        MaxRadius = (MaxRadius < r) ? r : MaxRadius;
    }

    // Draw Ellipses and DX DY point
    pen.setColor(Qt::blue);
    pen.setWidth(1);
    painter->setPen(pen);
    painter->drawEllipse(DPoint(dx, -dy),
                         static_cast<int>(MaxRadius * dScale),
                         static_cast<int>(MaxRadius * dScale));

    pen.setColor(Qt::gray);
    pen.setWidth(1);
    painter->setPen(pen);
    painter->drawEllipse(DPoint(dx, -dy),
                         static_cast<int>(MaxRadius * dScale * kxR),
                         static_cast<int>(MaxRadius * dScale * kyR));

    pen.setColor(Qt::red);
    pen.setWidth(7);
    painter->setPen(pen);
    painter->drawPoint(DPoint(dx, -dy));

    // Draw Vectors
    pen.setColor(Qt::cyan);
    pen.setWidth(1);
    painter->setPen(pen);
    painter->drawLine(DPoint(0, 0), DPoint(cVector));

    pen.setColor(QColor(255, 100, 100));
    pen.setWidth(1);
    painter->setPen(pen);
    painter->drawLine(DPoint(dx, -dy), DPoint(cVector));

    pen.setColor(Qt::green);
    pen.setWidth(7);
    painter->setPen(pen);
    painter->drawPoint(DPoint(cVector.rx(), cVector.ry()));
    painter->setPen(QPen());

    // Draw Text DATA
    pen.setColor(Qt::yellow);
    pen.setWidth(2);
    painter->setPen(pen);
    painter->drawText(QPointF(10, 10), QString("b%1 = %2").arg(sAxis[0]).arg(dx, 0, 'f', 3));
    painter->drawText(QPointF(10, 20), QString("b%1 = %2").arg(sAxis[1]).arg(dy, 0, 'f', 3));

    painter->drawText(QPointF(dOffset + 20, 10),
                      QString("s%1 = %2").arg(sAxis[0]).arg(1.0 / kxR, 0, 'f', 2));
    painter->drawText(QPointF(dOffset + 20, 20),
                      QString("s%1 = %2").arg(sAxis[1]).arg(1.0 / kyR, 0, 'f', 2));

    pen.setColor(Qt::lightGray);
    pen.setWidth(2);
    painter->setPen(pen);
    painter->drawText(QPointF(10, 30), QString("r: %1").arg(MaxRadius, 0, 'f', 2));

    pen.setColor(Qt::cyan);
    pen.setWidth(2);
    painter->setPen(pen);
    painter->drawText(QPointF(10, 40),
                      QString("a: %1%2").arg(bearingDegrees, 0, 'f', 0).arg(QChar(176)));

    pen.setColor(QColor(255, 100, 100));
    pen.setWidth(2);
    painter->setPen(pen);

    painter->drawText(QPointF(10, 50),
                      QString("b: %1%2").arg(bearingDegrees2, 0, 'f', 0).arg(QChar(176)));

    // ERROR - COlor changing
    int maxError = 10;
    int errcolor = (static_cast<int>(std::abs(angError)) * 255) / maxError;

    errcolor = (errcolor > 230) ? 230 : errcolor;
    pen.setColor(QColor(errcolor, 120 - errcolor / 2, 255 - errcolor));
    pen.setWidth(2);
    painter->setPen(pen);
    painter->drawText(QPointF(10, 60),
                      QString("e: %1%2").arg(AppRoot::angle(angError), 0, 'f', 0).arg(QChar(176)));
}
void DrawingArea::paintEvent(QPaintEvent *p)
{
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, 1);
    painter->drawPixmap(0, 0, *pixMap);
    DrawData();
    painter->end();
    QWidget::paintEvent(p);
}

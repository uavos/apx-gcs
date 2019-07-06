
#ifndef DRAWINGAREA_H
#define DRAWINGAREA_H
#include <QtWidgets>

class DrawingArea : public QWidget
{
    Q_OBJECT
public:
    DrawingArea();
    ~DrawingArea();

public:
    void resizeArea(int width, int height);
    void addData(double x, double y);
    void startTrace();
    void stopTrace();

    void clearTrace();

    QString sAxis[2];
    double bX, bY, sX, sY;

private:
    QPolygonF getPolygon(int k = 25);
    void DrawData();
    double dScale, dOffset;
    bool isTraced;
    QPointF cVector;
    QList<QPointF> cVectorList;
    double xMin, xMax, yMin, yMax;

private:
    QPixmap *pixMap;
    QPainter *painter;
    QPointF DPoint(double x, double y);
    QPointF DPoint(QPointF pt);

protected:
    void paintEvent(QPaintEvent *p);
    void resizeEvent(QResizeEvent *event);
    int heightForWidth(int w) const { return w; }
};

#endif // DRAWINGAREA_H

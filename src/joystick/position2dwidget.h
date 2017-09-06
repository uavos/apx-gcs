#ifndef POSITION2DWIDGET_H
#define POSITION2DWIDGET_H

#include <QWidget>
#include <QMouseEvent>

class Position2dWidget : public QWidget
{
    Q_OBJECT
public:
    explicit Position2dWidget(QWidget *parent = 0);
    void reset();
    void setEnabledXMoving(bool b);
    void setEnabledYMoving(bool b);
    int maximum() const;
    void setMaximum(int maximum);
    int minimum() const;
    void setMinimum(int minimum);

    const static int WIDGET_HEIGHT = 150;
    const static int WIDGET_WIDTH = 150;

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *e);

private:
    QPoint m_point;
    bool m_mousePress;
    bool m_XMoving;
    bool m_YMoving;
    int m_maximum;
    int m_minimum;
signals:
    void XChanged(double x);
    void YChanged(double y);
};

#endif // POSITION2DWIDGET_H

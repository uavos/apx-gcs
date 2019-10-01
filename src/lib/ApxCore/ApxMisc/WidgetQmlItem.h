#ifndef WidgetQmlItem_H
#define WidgetQmlItem_H

#include <QtQuick>

class QWidget;

class WidgetQmlItem : public QQuickPaintedItem
{
    Q_OBJECT

public:
    WidgetQmlItem(QQuickItem *parent = nullptr);
    virtual ~WidgetQmlItem() override;

    void paint(QPainter *painter) override;

    Q_INVOKABLE void setWidget(QWidget *w);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    /*//route mouse events
    virtual void hoverEnterEvent(QHoverEvent *event) override;
    virtual void hoverLeaveEvent(QHoverEvent *event) override;
    virtual void hoverMoveEvent(QHoverEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

    void routeHoverEvents(QHoverEvent *event);
    void routeMouseEvents(QMouseEvent *event);
    void routeWheelEvents(QWheelEvent *event);*/

    virtual bool event(QEvent *e) override;

private:
    QWidget *m_widget;

private slots:
    void updateWidgetSize();
};

#endif

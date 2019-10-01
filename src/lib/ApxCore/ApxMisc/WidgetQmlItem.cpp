#include "WidgetQmlItem.h"
#include <QAbstractScrollArea>
#include <QApplication>
#include <QWidget>

WidgetQmlItem::WidgetQmlItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_widget(nullptr)
{
    //setFlag(QQuickItem::ItemHasContents, true);
    //setOpaquePainting(true);
    //setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);

    connect(this, &QQuickPaintedItem::widthChanged, this, &WidgetQmlItem::updateWidgetSize);
    connect(this, &QQuickPaintedItem::heightChanged, this, &WidgetQmlItem::updateWidgetSize);
}

WidgetQmlItem::~WidgetQmlItem()
{
    delete m_widget;
    m_widget = nullptr;
}

void WidgetQmlItem::setWidget(QWidget *w)
{
    m_widget = w;
    m_widget->installEventFilter(this);
    updateWidgetSize();
    update();

    //QTimer::singleShot(0, this, &QQuickPaintedItem::update);
}

bool WidgetQmlItem::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_widget) {
        switch (event->type()) {
        case QEvent::Paint:
        case QEvent::UpdateRequest:
            update();
            break;
        default:
            break;
        }
    }
    QQuickPaintedItem::eventFilter(obj, event);
    return false;
}

void WidgetQmlItem::paint(QPainter *painter)
{
    if (m_widget) {
        m_widget->render(painter);
    }
}

bool WidgetQmlItem::event(QEvent *e)
{
    switch (e->type()) {
    default:
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::Wheel:
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        qDebug() << "SEND EVENT:" << e;
        QApplication::sendEvent(m_widget, e);
        QAbstractScrollArea *s = qobject_cast<QAbstractScrollArea *>(m_widget);
        if (s) {
            QApplication::sendEvent(s->viewport(), e);
        }
        return true;
    }
    case QEvent::FocusIn:
        this->forceActiveFocus();
        break;
    }
    return QQuickPaintedItem::event(e);
}

/*void WidgetQmlItem::hoverEnterEvent(QHoverEvent *event)
{
    routeHoverEvents(event);
}
void WidgetQmlItem::hoverLeaveEvent(QHoverEvent *event)
{
    routeHoverEvents(event);
}
void WidgetQmlItem::hoverMoveEvent(QHoverEvent *event)
{
    routeHoverEvents(event);
}

void WidgetQmlItem::mousePressEvent(QMouseEvent *event)
{
    routeMouseEvents(event);
}

void WidgetQmlItem::mouseReleaseEvent(QMouseEvent *event)
{
    routeMouseEvents(event);
}

void WidgetQmlItem::mouseMoveEvent(QMouseEvent *event)
{
    routeMouseEvents(event);
}

void WidgetQmlItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    routeMouseEvents(event);
}

void WidgetQmlItem::wheelEvent(QWheelEvent *event)
{
    routeWheelEvents(event);
}

void WidgetQmlItem::routeHoverEvents(QHoverEvent *event)
{
    if (m_widget) {
        QHoverEvent *newEvent = new QHoverEvent(event->type(),
                                                event->pos(),
                                                event->oldPosF(),
                                                event->modifiers());
        QCoreApplication::postEvent(m_widget, newEvent);
    }
}

void WidgetQmlItem::routeMouseEvents(QMouseEvent *event)
{
    if (m_widget) {
        QMouseEvent *newEvent = new QMouseEvent(event->type(),
                                                event->localPos(),
                                                event->button(),
                                                event->buttons(),
                                                event->modifiers());
        QCoreApplication::postEvent(m_widget, newEvent);
    }
    update();
}

void WidgetQmlItem::routeWheelEvents(QWheelEvent *event)
{
    if (m_widget) {
        QWheelEvent *newEvent = new QWheelEvent(event->pos(),
                                                event->delta(),
                                                event->buttons(),
                                                event->modifiers(),
                                                event->orientation());
        QCoreApplication::postEvent(m_widget, newEvent);
    }
    update();
}*/

void WidgetQmlItem::updateWidgetSize()
{
    if (m_widget) {
        m_widget->setGeometry(0, 0, static_cast<int>(width()), static_cast<int>(height()));
    }
}

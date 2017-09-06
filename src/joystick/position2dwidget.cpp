#include "position2dwidget.h"
#include <QPainter>

Position2dWidget::Position2dWidget(QWidget *parent) :
    QWidget(parent),
    m_point(WIDGET_WIDTH / 2, WIDGET_HEIGHT / 2),
    m_mousePress(false),
    m_XMoving(false),
    m_YMoving(false),
    m_maximum(1),
    m_minimum(-1)
{
    hide();
    setFixedSize(WIDGET_WIDTH, WIDGET_HEIGHT);
    setMouseTracking(true);
}

void Position2dWidget::reset()
{
    m_point.setX(WIDGET_WIDTH / 2);
    m_point.setY(WIDGET_HEIGHT / 2);
    m_XMoving = false;
    m_YMoving = false;
    update();
}

void Position2dWidget::setEnabledXMoving(bool b)
{
    m_XMoving = b;
}

void Position2dWidget::setEnabledYMoving(bool b)
{
    m_YMoving = b;
}

void Position2dWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setBrush(Qt::SolidPattern);
    painter.drawEllipse(m_point, 4, 4);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(1, 1, WIDGET_WIDTH - 2, WIDGET_HEIGHT - 2);
}

void Position2dWidget::mouseMoveEvent(QMouseEvent *e)
{
    if(m_mousePress && e->pos().x() <= WIDGET_WIDTH && e->pos().y() <= WIDGET_HEIGHT && e->pos().x() >= 0 && e->pos().y() >= 0)
    {
        if(e->pos().x() != m_point.x() && m_XMoving)
        {
            emit XChanged(double(e->pos().x() * (m_maximum - m_minimum)) / double(WIDGET_WIDTH) + m_minimum);
            m_point.setX(e->pos().x());
        }
        if(e->pos().y() != m_point.y() && m_YMoving)
        {
            emit YChanged(double((WIDGET_HEIGHT - e->pos().y()) * (m_maximum - m_minimum)) / double(WIDGET_HEIGHT) + m_minimum);
            m_point.setY(e->pos().y());
        }
    }
    update();
}

void Position2dWidget::mousePressEvent(QMouseEvent *e)
{
    m_mousePress = true;
    if(e->pos().x() != m_point.x() && m_XMoving)
    {
        emit XChanged(double(e->pos().x() * (m_maximum - m_minimum)) / double(WIDGET_WIDTH) + m_minimum);
        m_point.setX(e->pos().x());
    }
    if(e->pos().y() != m_point.y() && m_YMoving)
    {
        emit YChanged(double((WIDGET_HEIGHT - e->pos().y()) * (m_maximum - m_minimum)) / double(WIDGET_HEIGHT) + m_minimum);
        m_point.setY(e->pos().y());
    }
    update();
}

void Position2dWidget::mouseReleaseEvent(QMouseEvent *e)
{
    m_point.setX(WIDGET_WIDTH / 2);
    m_point.setY(WIDGET_HEIGHT / 2);
    if(e->pos().x() != m_point.x() && m_XMoving)
        emit XChanged(double(m_point.x() * (m_maximum - m_minimum)) / double(WIDGET_WIDTH) + m_minimum);
    if(e->pos().y() != m_point.y() && m_YMoving)
        emit YChanged(double((WIDGET_HEIGHT - m_point.y()) * (m_maximum - m_minimum)) / double(WIDGET_HEIGHT) + m_minimum);
    m_mousePress = false;
    update();
}

int Position2dWidget::minimum() const
{
    return m_minimum;
}

void Position2dWidget::setMinimum(int minimum)
{
    m_minimum = minimum;
}

int Position2dWidget::maximum() const
{
    return m_maximum;
}

void Position2dWidget::setMaximum(int maximum)
{
    m_maximum = maximum;
}

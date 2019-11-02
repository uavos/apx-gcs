#include "QmlAnimationDriver.h"

QmlAnimationDriver::QmlAnimationDriver(int msPerStep)
    : m_step(msPerStep)
    , m_elapsed(0)
{}

void QmlAnimationDriver::advance()
{
    m_elapsed += m_step;
    advanceAnimation();
}

qint64 QmlAnimationDriver::elapsed() const
{
    return m_elapsed;
}

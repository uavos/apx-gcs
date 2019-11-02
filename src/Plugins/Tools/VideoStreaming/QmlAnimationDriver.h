#ifndef QmlAnimationDriver_H
#define QmlAnimationDriver_H

#include <QtCore/QAnimationDriver>

class QmlAnimationDriver : public QAnimationDriver
{
public:
    QmlAnimationDriver(int msPerStep);

    void advance() override;
    qint64 elapsed() const override;

private:
    int m_step;
    qint64 m_elapsed;
};

#endif

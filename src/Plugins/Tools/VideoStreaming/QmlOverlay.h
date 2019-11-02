#ifndef QmlOverlay_H
#define QmlOverlay_H

#include <QImage>
#include <QtCore>

class QmlRenderer;
class QmlOverlay : public QThread
{
    Q_OBJECT
public:
    QmlOverlay(QObject *parent = nullptr);

    void drawOverlay(QImage &image);

    void run() override;

private:
    QImage overlay;
    QmlRenderer *renderer;

private slots:
    void imageRendered(const QImage &image);

signals:
    void frameUpdated();
};

#endif

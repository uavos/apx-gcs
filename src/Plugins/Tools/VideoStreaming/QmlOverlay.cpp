#include "QmlOverlay.h"
#include "QmlRenderer.h"

#include <QPainter>

QmlOverlay::QmlOverlay(QObject *parent)
    : QThread(parent)
//, overlay(100, 100, QImage::Format_ARGB32)
{
    start();
}

void QmlOverlay::run()
{
    renderer = new QmlRenderer;
    QObject::connect(renderer, &QmlRenderer::imageRendered, this, &QmlOverlay::imageRendered);

    QString sourceFile = QString("qrc:/%1/Overlay.qml").arg(PLUGIN_NAME);
    renderer->loadQmlFile(sourceFile, QSize(100, 100));

    connect(this, &QmlOverlay::frameUpdated, renderer, &QmlRenderer::renderNext);

    exec();
}

void QmlOverlay::imageRendered(const QImage &image)
{
    //        const auto mono = image.convertToFormat(QImage::Format_Mono,
    //                                                Qt::MonoOnly | Qt::ThresholdDither);
    //        driver.writeImage(mono);
    overlay = image; //.convertToFormat(overlay.format());
    qDebug() << image;
}

void QmlOverlay::drawOverlay(QImage &image)
{
    //qDebug() << overlay;
    QPainter painter(&image);
    painter.save();
    painter.setPen(Qt::red);
    painter.drawLine(0, 0, 100, 100);
    painter.drawImage(0, 0, overlay);
    //painter.drawImage(0, 0, renderer->renderImage());
    //image = overlay.convertToFormat(image.format());

    painter.restore();
    emit frameUpdated();
}

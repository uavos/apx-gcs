#ifndef QmlOverlayMain_H
#define QmlOverlayMain_H

#include <QImage>
#include <QtCore>
#include <QtQuick>

class QmlOverlayMain : public QObject
{
    Q_OBJECT
public:
    explicit QmlOverlayMain(QObject *parent = nullptr);
    ~QmlOverlayMain();

    void cb_drawOverlay(QImage &image);

signals:
    void imageRendered(const QImage &image);
    void videoFrameUpdated();

private slots:
    void createFbo();
    void destroyFbo();
    bool loadQml(const QString &qmlFile, const QSize &size);

public slots:
    void renderNext();

private:
    QOpenGLContext *m_context;
    QOffscreenSurface *m_offscreenSurface;
    QQuickRenderControl *m_renderControl;
    QQuickWindow *m_quickWindow;
    QQmlEngine *m_qmlEngine;
    QQmlComponent *m_qmlComponent;
    QQuickItem *m_rootItem;
    QOpenGLFramebufferObject *m_fbo;
    qreal m_dpr;
    QSize m_size;

    //QMutex mutex;
    QImage overlay;

    void loadQmlFile(const QString &qmlFile, const QSize &size, qreal devicePixelRatio = 1.0);
};

#endif

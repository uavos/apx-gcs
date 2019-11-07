#ifndef QmlOverlay_H
#define QmlOverlay_H

#include <QImage>
#include <QtCore>
#include <QtQuick>

class QmlOverlay : public QObject
{
    Q_OBJECT
public:
    explicit QmlOverlay(QObject *parent = nullptr);
    ~QmlOverlay();

    void cb_drawOverlay(QImage &image);

    QImage getSnapshotOverlay(const QSize &size);

signals:
    void imageRendered(const QImage &image);
    void renderRequest();
    void resizeRequest(QSize size);

private slots:

    void sceneChanged();
    void resizeRootItem(QSize size);

    bool loadQml(const QString &qmlFile);

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

    bool m_needPolishAndSync;

    QMutex mutex;
    QImage cb_overlay;

    QTimer timer;

    void loadQmlFile(const QString &qmlFile, const QSize &size, qreal devicePixelRatio = 1.0);

    void createFbo(const QSize &size);
    void destroyFbo();
};

#endif

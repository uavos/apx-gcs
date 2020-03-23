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
    QOpenGLContext *m_context{nullptr};
    QOffscreenSurface *m_offscreenSurface{nullptr};
    QQuickRenderControl *m_renderControl{nullptr};
    QQuickWindow *m_quickWindow{nullptr};
    QQmlComponent *m_qmlComponent{nullptr};
    QQuickItem *m_rootItem{nullptr};
    QOpenGLFramebufferObject *m_fbo{nullptr};
    qreal m_dpr{1.0};

    bool m_needPolishAndSync{true};

    QMutex mutex;
    QImage cb_overlay;

    QTimer timer;

    void loadQmlFile(const QString &qmlFile, const QSize &size, qreal devicePixelRatio = 1.0);

    void createFbo(const QSize &size);
    void destroyFbo();
};

#endif

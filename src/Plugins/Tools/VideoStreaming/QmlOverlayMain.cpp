#include "QmlOverlayMain.h"

#include <App/App.h>
#include <QSurfaceFormat>

QmlOverlayMain::QmlOverlayMain(QObject *parent)
    : QObject(parent)
    , m_context(nullptr)
    , m_offscreenSurface(nullptr)
    , m_renderControl(nullptr)
    , m_quickWindow(nullptr)
    , m_qmlEngine(nullptr)
    , m_qmlComponent(nullptr)
    , m_rootItem(nullptr)
    , m_fbo(nullptr)
{
    QOpenGLContext *extContext = QOpenGLContext::currentContext();

    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    format.setSamples(1);

    m_context = new QOpenGLContext;
    m_context->setFormat(format);
    m_context->setShareContext(extContext);
    m_context->create();

    m_offscreenSurface = new QOffscreenSurface;
    m_offscreenSurface->setFormat(m_context->format());
    m_offscreenSurface->create();

    m_renderControl = new QQuickRenderControl(this);
    m_quickWindow = new QQuickWindow(m_renderControl);
    m_quickWindow->setColor(QColor(0, 0, 0, 0));

    m_qmlEngine = App::instance()->engine();

    m_context->makeCurrent(m_offscreenSurface);
    m_renderControl->initialize(m_context);
    m_context->doneCurrent();

    QString sourceFile = QString("qrc:/%1/Overlay.qml").arg(PLUGIN_NAME);
    loadQmlFile(sourceFile, QSize(100, 100));

    connect(this, &QmlOverlayMain::videoFrameUpdated, this, &QmlOverlayMain::renderNext);
}

QmlOverlayMain::~QmlOverlayMain()
{
    m_context->makeCurrent(m_offscreenSurface);
    delete m_renderControl;
    delete m_qmlComponent;
    delete m_quickWindow;
    delete m_fbo;
    m_context->doneCurrent();

    delete m_offscreenSurface;
    delete m_context;
}

void QmlOverlayMain::cb_drawOverlay(QImage &image)
{
    //mutex.lock();

    //mutex.unlock();
    //qDebug() << image;
    QPainter painter(&image);
    painter.save();
    painter.setPen(Qt::red);
    painter.drawLine(0, 0, 100, 100);
    painter.drawImage(0, 0, overlay);

    painter.restore();
    emit videoFrameUpdated();
}

void QmlOverlayMain::loadQmlFile(const QString &qmlFile, const QSize &size, qreal devicePixelRatio)
{
    m_size = size;
    m_dpr = devicePixelRatio;

    if (!loadQml(qmlFile, size)) {
        return;
    }

    createFbo();
    renderNext();
}

void QmlOverlayMain::createFbo()
{
    m_context->makeCurrent(m_offscreenSurface);
    m_fbo = new QOpenGLFramebufferObject(m_size * m_dpr,
                                         QOpenGLFramebufferObject::CombinedDepthStencil);
    m_quickWindow->setRenderTarget(m_fbo);
    m_context->doneCurrent();
}

void QmlOverlayMain::destroyFbo()
{
    delete m_fbo;
    m_fbo = nullptr;
}

bool QmlOverlayMain::loadQml(const QString &qmlFile, const QSize &size)
{
    if (m_qmlComponent != nullptr) {
        delete m_qmlComponent;
    }
    m_qmlComponent = new QQmlComponent(m_qmlEngine, QUrl(qmlFile), QQmlComponent::PreferSynchronous);

    if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList)
            qWarning() << error.url() << error.line() << error;
        return false;
    }

    QObject *rootObject = m_qmlComponent->create();
    if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList)
            qWarning() << error.url() << error.line() << error;
        return false;
    }

    m_rootItem = qobject_cast<QQuickItem *>(rootObject);
    if (!m_rootItem) {
        qWarning() << "missing root item";
        delete rootObject;
        return false;
    }

    // The root item is ready. Associate it with the window.
    m_rootItem->setParentItem(m_quickWindow->contentItem());

    m_rootItem->setWidth(size.width());
    m_rootItem->setHeight(size.height());

    m_quickWindow->setGeometry(0, 0, size.width(), size.height());

    return true;
}

void QmlOverlayMain::renderNext()
{
    if (!m_fbo)
        return;
    if (!m_context->makeCurrent(m_offscreenSurface))
        return;

    m_renderControl->polishItems();
    m_renderControl->sync();
    m_renderControl->render();

    m_context->functions()->glFlush();
    overlay = m_fbo->toImage();

    m_context->doneCurrent();

    //mutex.lock();
    //overlay = image;
    //mutex.unlock();
}

#include "QmlOverlay.h"

#include <App/App.h>
#include <QSurfaceFormat>

QmlOverlay::QmlOverlay(QObject *parent)
    : QObject(parent)
    , m_context(nullptr)
    , m_offscreenSurface(nullptr)
    , m_renderControl(nullptr)
    , m_quickWindow(nullptr)
    , m_qmlEngine(nullptr)
    , m_qmlComponent(nullptr)
    , m_rootItem(nullptr)
    , m_fbo(nullptr)
    , m_needPolishAndSync(true)
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
    connect(m_renderControl, &QQuickRenderControl::sceneChanged, this, &QmlOverlay::sceneChanged);

    m_qmlEngine = App::instance()->engine();

    m_context->makeCurrent(m_offscreenSurface);
    m_renderControl->initialize(m_context);
    m_context->doneCurrent();

    loadQmlFile(QString("qrc:/%1/Overlay.qml").arg(PLUGIN_NAME), QSize(100, 100));

    connect(this,
            &QmlOverlay::resizeRequest,
            this,
            &QmlOverlay::resizeRootItem,
            Qt::QueuedConnection);

    timer.setSingleShot(true);
    timer.setInterval(20);
    connect(&timer, &QTimer::timeout, this, &QmlOverlay::renderNext);
    connect(
        this,
        &QmlOverlay::renderRequest,
        &timer,
        [this]() {
            if (!timer.isActive())
                timer.start();
        },
        Qt::QueuedConnection);
}

QmlOverlay::~QmlOverlay()
{
    disconnect(this, nullptr, this, nullptr);

    m_context->makeCurrent(m_offscreenSurface);
    delete m_renderControl;
    delete m_qmlComponent;
    delete m_quickWindow;
    delete m_fbo;
    m_context->doneCurrent();

    delete m_offscreenSurface;
    delete m_context;
}

void QmlOverlay::loadQmlFile(const QString &qmlFile, const QSize &size, qreal devicePixelRatio)
{
    m_dpr = devicePixelRatio;

    if (!loadQml(qmlFile)) {
        return;
    }

    resizeRootItem(size);
    createFbo(size);
    renderNext();
}

void QmlOverlay::createFbo(const QSize &size)
{
    m_context->makeCurrent(m_offscreenSurface);
    m_fbo = new QOpenGLFramebufferObject(size * m_dpr,
                                         QOpenGLFramebufferObject::CombinedDepthStencil);
    m_quickWindow->setRenderTarget(m_fbo);
    m_context->doneCurrent();
}

void QmlOverlay::destroyFbo()
{
    delete m_fbo;
    m_fbo = nullptr;
}

bool QmlOverlay::loadQml(const QString &qmlFile)
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

    m_rootItem->setParentItem(m_quickWindow->contentItem());

    return true;
}

void QmlOverlay::sceneChanged()
{
    m_needPolishAndSync = true;
    emit renderRequest();
}

void QmlOverlay::cb_drawOverlay(QImage &image)
{
    mutex.lock();
    QSize size = cb_overlay.size();
    mutex.unlock();

    if (image.size() != size) {
        emit resizeRequest(image.size());
        return;
    }
    //qDebug() << image;
    QPainter painter(&image);
    //    painter.save();
    //    painter.setPen(Qt::red);
    //    painter.drawLine(0, 0, 100, 100);
    mutex.lock();
    painter.drawImage(QPoint(0, 0), cb_overlay);
    mutex.unlock();

    //    painter.restore();
    emit renderRequest();
}

void QmlOverlay::renderNext()
{
    if (!m_fbo)
        return;
    if (!m_context)
        return;
    if (!m_rootItem)
        return;

    QSize size = m_rootItem->size().toSize();
    if (m_fbo->size() != size) {
        //qDebug() << size;
        if (!m_context->makeCurrent(m_offscreenSurface))
            return;
        destroyFbo();
        createFbo(size);
        m_context->doneCurrent();
        m_needPolishAndSync = true;
    }

    if (!m_context->makeCurrent(m_offscreenSurface))
        return;

    if (m_needPolishAndSync) {
        m_needPolishAndSync = false;
        //qDebug() << m_renderControl;
        m_renderControl->polishItems();
        m_renderControl->sync();
    }
    m_renderControl->render();

    m_context->functions()->glFlush();
    mutex.lock();
    cb_overlay = m_fbo->toImage();
    mutex.unlock();

    m_context->doneCurrent();
}

void QmlOverlay::resizeRootItem(QSize size)
{
    if (m_rootItem) {
        if (m_rootItem->size().toSize() == size)
            return;
        m_rootItem->setWidth(size.width());
        m_rootItem->setHeight(size.height());
        m_quickWindow->setGeometry(0, 0, size.width(), size.height());
    }
    emit renderRequest();
}

QImage QmlOverlay::getSnapshotOverlay(const QSize &size)
{
    resizeRootItem(size);
    renderNext();
    mutex.lock();
    QImage image = cb_overlay;
    mutex.unlock();
    return image;
}

/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "QmlOverlay.h"

#include <App/App.h>
#include <QSurfaceFormat>

// https://forum.qt.io/topic/155040/qt6-render-qml-to-images-using-qquickrendercontrol/3
// https://www.qtcentre.org/threads/72013-Rendering-a-QQuickItem-into-a-QImage-(Qt6)

QmlOverlay::QmlOverlay(QObject *parent)
    : QObject(parent)
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

    m_renderControl = new QQuickRenderControl;
    m_quickWindow = new QQuickWindow(m_renderControl);
    m_quickWindow->setColor(QColor(0, 0, 0, 0));
    connect(m_renderControl, &QQuickRenderControl::sceneChanged, this, &QmlOverlay::sceneChanged);

    if (!m_context->isValid())
        return;

    m_context->makeCurrent(m_offscreenSurface);
    m_renderControl->initialize();
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
    disconnect(this);
    disconnect();

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
    m_quickWindow->setGraphicsApi(QSGRendererInterface::OpenGL);

    m_context->makeCurrent(m_offscreenSurface);
    m_fbo = new QOpenGLFramebufferObject(size * m_dpr,
                                         QOpenGLFramebufferObject::CombinedDepthStencil);

    QQuickRenderTarget rt = QQuickRenderTarget::fromOpenGLRenderBuffer(m_fbo->handle(),
                                                                       m_fbo->size(),
                                                                       1);

    // QQuickRenderTarget rt = QQuickRenderTarget::fromVulkanImage(vulkanImage,
    //                                                             VK_IMAGE_LAYOUT_PREINITIALIZED,
    //                                                             m_fbo->size(),
    //                                                             1);

    m_quickWindow->setRenderTarget(rt);
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
    AppEngine *e = App::instance()->engine();
    if (!e)
        return false;

    m_qmlComponent = new QQmlComponent(e, QUrl(qmlFile), QQmlComponent::PreferSynchronous);

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

    // m_quickWindow->beginExternalCommands(); // Begin external OpenGL commands
    m_renderControl->beginFrame();
    if (m_needPolishAndSync) {
        m_needPolishAndSync = false;
        //qDebug() << m_renderControl;
        m_renderControl->polishItems();
        m_renderControl->sync();
    }
    m_renderControl->render();
    m_renderControl->endFrame();
    // m_quickWindow->endExternalCommands(); // End external OpenGL commands

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

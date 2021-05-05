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
#pragma once

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

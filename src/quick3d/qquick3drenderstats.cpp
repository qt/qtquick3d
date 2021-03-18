/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquick3drenderstats_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype RenderStats
    \inqmlmodule QtQuick3D
    \brief Provides information of the scene rendering.

    Uncreatable accessor to scene rendering statistics.
*/

QQuick3DRenderStats::QQuick3DRenderStats(QObject *parent)
    : QObject(parent)
{
    m_frameTimer.start();
}

/*!
    \qmlproperty int QtQuick3D::RenderStats::fps

    This property holds the amount of frames rendered
    during the last second i.e. frames per second.
*/
int QQuick3DRenderStats::fps() const
{
    return m_fps;
}

/*!
    \qmlproperty float QtQuick3D::RenderStats::frameTime

    This property holds the amount of time elapsed since the last frame.
*/
float QQuick3DRenderStats::frameTime() const
{
    return m_frameTime;
}

/*!
    \qmlproperty float QtQuick3D::RenderStats::renderTime

    This property holds the amount of time spent inside the render function.
*/
float QQuick3DRenderStats::renderTime() const
{
    return m_renderTime;
}

/*!
    \qmlproperty float QtQuick3D::RenderStats::syncTime

    This property holds the amount of time spent inside the sync function.
    The property values of the objects are updated during the sync.
*/
float QQuick3DRenderStats::syncTime() const
{
    return m_syncTime;
}

/*!
    \qmlproperty float QtQuick3D::RenderStats::maxFrameTime

    This property holds the maximum time spent rendering a single frame during the last second.
*/
float QQuick3DRenderStats::maxFrameTime() const
{
    return m_maxFrameTime;
}

void QQuick3DRenderStats::startSync()
{
    m_syncStartTime = timestamp();
}

void QQuick3DRenderStats::endSync(bool dump)
{
    m_syncTime = timestamp() - m_syncStartTime;

    if (dump)
        qDebug() << "Sync took: " << m_syncTime << "ms";
}

void QQuick3DRenderStats::startRender()
{
    m_renderStartTime = timestamp();
}

void QQuick3DRenderStats::endRender(bool dump)
{
    ++m_frameCount;
    m_frameTime = timestamp();
    m_internalMaxFrameTime = qMax(m_frameTime, m_internalMaxFrameTime);

    m_secTimer += m_frameTime;
    m_notifyTimer += m_frameTime;

    m_renderTime = m_frameTime - m_renderStartTime;

    const float notifyInterval = 200.0f;
    if (m_notifyTimer >= notifyInterval) {
        m_notifyTimer -= notifyInterval;

        if (m_frameTime != m_notifiedFrameTime) {
            m_notifiedFrameTime = m_frameTime;
            emit frameTimeChanged();
        }

        if (m_syncTime != m_notifiedSyncTime) {
            m_notifiedSyncTime = m_syncTime;
            emit syncTimeChanged();
        }

        if (m_renderTime != m_notifiedRenderTime) {
            m_notifiedRenderTime = m_renderTime;
            emit renderTimeChanged();
        }
    }

    const float fpsInterval = 1000.0f;
    if (m_secTimer >= fpsInterval) {
        m_secTimer -= fpsInterval;

        m_fps = m_frameCount;
        m_frameCount = 0;
        emit fpsChanged();

        m_maxFrameTime = m_internalMaxFrameTime;
        m_internalMaxFrameTime = 0;
        emit maxFrameTimeChanged();
    }

    m_frameTimer.restart();

    if (dump)
        qDebug() << "Render took: " << m_renderTime << "ms";
}

float QQuick3DRenderStats::timestamp() const
{
    return m_frameTimer.nsecsElapsed() / 1000000.0f;
}

QT_END_NAMESPACE

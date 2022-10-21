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

    The RenderStats type provides information about scene rendering statistics. This
    cannot be created directly, but can be retrieved from a \l View3D.
*/

QQuick3DRenderStats::QQuick3DRenderStats(QObject *parent)
    : QObject(parent)
{
    m_frameTimer.start();
}

/*!
    \qmlproperty int QtQuick3D::RenderStats::fps
    \readonly

    This property holds the number of frames rendered during the last second.
*/
int QQuick3DRenderStats::fps() const
{
    return m_fps;
}

/*!
    \qmlproperty float QtQuick3D::RenderStats::frameTime
    \readonly

    This property holds the amount of time elapsed since the last frame, in
    milliseconds.
*/
float QQuick3DRenderStats::frameTime() const
{
    return m_results.frameTime;
}

/*!
    \qmlproperty float QtQuick3D::RenderStats::renderTime
    \readonly

    This property holds the amount of time spent on generating a new frame,
    including both the preparation phase and the recording of draw calls. The
    value is in milliseconds.
*/
float QQuick3DRenderStats::renderTime() const
{
    return m_results.renderTime;
}

/*!
    \qmlproperty float QtQuick3D::RenderStats::renderPrepareTime
    \readonly

    This property holds the amount of time spent in the preparation phase of
    rendering, in milliseconds. This is a subset of the total render time
    reported in renderTime.
*/
float QQuick3DRenderStats::renderPrepareTime() const
{
    return m_results.renderPrepareTime;
}

/*!
    \qmlproperty float QtQuick3D::RenderStats::syncTime
    \readonly

    This property holds the amount of time spent inside the sync function, in
    milliseconds. The property values of the objects are updated during the
    sync.
*/
float QQuick3DRenderStats::syncTime() const
{
    return m_results.syncTime;
}

/*!
    \qmlproperty float QtQuick3D::RenderStats::maxFrameTime
    \readonly

    This property holds the maximum time spent rendering a single frame during
    the last second.
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
    m_results.syncTime = timestamp() - m_syncStartTime;

    if (dump)
        qDebug("Sync took: %f ms", m_results.syncTime);
}

void QQuick3DRenderStats::startRender()
{
    m_renderStartTime = timestamp();
}

void QQuick3DRenderStats::startRenderPrepare()
{
    m_renderPrepareStartTime = timestamp();
}

void QQuick3DRenderStats::endRenderPrepare()
{
    m_results.renderPrepareTime = timestamp() - m_renderPrepareStartTime;
}

void QQuick3DRenderStats::endRender(bool dump)
{
    ++m_frameCount;
    m_results.frameTime = timestamp();
    m_internalMaxFrameTime = qMax(m_results.frameTime, m_internalMaxFrameTime);

    m_secTimer += m_results.frameTime;
    m_notifyTimer += m_results.frameTime;

    m_results.renderTime = m_results.frameTime - m_renderStartTime;

    const float notifyInterval = 200.0f;
    if (m_notifyTimer >= notifyInterval) {
        m_notifyTimer -= notifyInterval;

        if (m_results.frameTime != m_notifiedResults.frameTime) {
            m_notifiedResults.frameTime = m_results.frameTime;
            emit frameTimeChanged();
        }

        if (m_results.syncTime != m_notifiedResults.syncTime) {
            m_notifiedResults.syncTime = m_results.syncTime;
            emit syncTimeChanged();
        }

        if (m_results.renderTime != m_notifiedResults.renderTime) {
            m_notifiedResults.renderTime = m_results.renderTime;
            m_notifiedResults.renderPrepareTime = m_results.renderPrepareTime;
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
        qDebug("Render took: %f ms (of which prep: %f ms)", m_results.renderTime, m_results.renderPrepareTime);
}

float QQuick3DRenderStats::timestamp() const
{
    return m_frameTimer.nsecsElapsed() / 1000000.0f;
}

QT_END_NAMESPACE

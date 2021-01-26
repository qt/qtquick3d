/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSSG_RENDER_PROFILER_H
#define QSSG_RENDER_PROFILER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>

QT_BEGIN_NAMESPACE

/**
 *	Opaque profiling system for rendering.
 */

class QSSGRenderContextInterface;
class QSSGRenderContext;
struct QSSGGpuTimerInfo;

class QSSGRenderGPUProfiler
{
private:
    QSSGRef<QSSGRenderContext> m_renderContext;
    QSSGRef<QSSGRenderContextInterface> m_context;

    typedef QHash<QString, QSSGRef<QSSGGpuTimerInfo>> TStrGpuTimerInfoMap;

    TStrGpuTimerInfoMap m_strToGpuTimerMap;
    QVector<QString> m_timerIds;
    mutable quint32 m_vertexCount;

    QSSGRef<QSSGGpuTimerInfo> getOrCreateGpuTimerInfo(QString &nameID);
    QSSGRef<QSSGGpuTimerInfo> getGpuTimerInfo(const QString &nameID) const;

public:

    QSSGRenderGPUProfiler(const QSSGRef<QSSGRenderContextInterface> &inContext, const QSSGRef<QSSGRenderContext> &inRenderContext);
    ~QSSGRenderGPUProfiler();

    /**
     * @brief start a timer query
     *
     * @param[in] nameID			Timer ID for tracking
     * @param[in] absoluteTime		If true the absolute GPU is queried
     * @param[in] sync				Do a sync before starting the timer
     *
     * @return no return
     */
    void startTimer(QString &nameID, bool absoluteTime, bool sync);

    /**
     * @brief stop a timer query
     *
     * @param[in] nameID			Timer ID for tracking
     *
     * @return no return
     */
    void endTimer(QString &nameID);

    /**
     * @brief Get elapsed timer value. Not this is an averaged time over several frames
     *
     * @param[in] nameID			Timer ID for tracking
     *
     * @return no return
     */
    double elapsed(const QString &nameID) const;

    /**
     * @brief Get ID list of tracked timers
     *
     * @return ID list
     */
    const QVector<QString> &timerIDs() const;

    /**
     * @brief add vertex count to current counter
     *
     * @return
     */
    void addVertexCount(quint32 count);

    /**
     * @brief get current vertex count and reset
     *
     * @return
     */
    quint32 getAndResetVertexCount() const;
};
QT_END_NAMESPACE

#endif

/****************************************************************************
**
** Copyright (C) 2008-2014 NVIDIA Corporation.
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

#include <QtQuick3DRuntimeRender/private/qssgrendergpuprofiler_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRender/private/qssgrendertimerquery_p.h>
#include <QtQuick3DRender/private/qssgrendersync_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>

#define RECORDED_FRAME_DELAY 3
#define RECORDED_FRAME_DELAY_MASK 0x0003

QT_BEGIN_NAMESPACE

struct QSSGGpuTimerInfo
{
    QAtomicInt ref;
    bool m_absoluteTime = false;
    quint8 m_writeID = 0;
    quint8 m_readID = 0;
    quint8 m_averageTimeWriteID = 0;
    quint64 m_averageTime[10];
    quint32 m_frameID[RECORDED_FRAME_DELAY];
    QSSGRef<QSSGRenderTimerQuery> m_timerStartQueryObjects[RECORDED_FRAME_DELAY];
    QSSGRef<QSSGRenderTimerQuery> m_timerEndQueryObjects[RECORDED_FRAME_DELAY];
    QSSGRef<QSSGRenderSync> m_timerSyncObjects[RECORDED_FRAME_DELAY];

    QSSGGpuTimerInfo() { memset(m_averageTime, 0x0, 10 * sizeof(quint64)); }

    void incrementWriteCounter()
    {
        m_writeID++;
        m_writeID %= RECORDED_FRAME_DELAY_MASK;
    }

    void incrementReadCounter()
    {
        m_readID++;
        m_readID %= RECORDED_FRAME_DELAY_MASK;
    }

    void incrementAveragedWriteCounter()
    {
        m_averageTimeWriteID++;
        m_averageTimeWriteID %= 10;
    }

    void startTimerQuery(quint32 frameID)
    {
        m_frameID[m_writeID] = frameID;

        if (m_absoluteTime)
            m_timerStartQueryObjects[m_writeID]->setTimerQuery();
        else
            m_timerStartQueryObjects[m_writeID]->begin();
    }

    void endTimerQuery()
    {
        if (m_absoluteTime)
            m_timerEndQueryObjects[m_writeID]->setTimerQuery();
        else
            m_timerStartQueryObjects[m_writeID]->end();

        incrementWriteCounter();
    }

    void addSync()
    {
        m_timerSyncObjects[m_writeID]->sync();
        m_timerSyncObjects[m_writeID]->wait();
    }

    double averagedElapsedTimeInMs()
    {
        double time = double((m_averageTime[0] + m_averageTime[1] + m_averageTime[2] + m_averageTime[3] + m_averageTime[4]
                               + m_averageTime[5] + m_averageTime[6] + m_averageTime[7] + m_averageTime[8] + m_averageTime[9])
                              / 10)
                             / double(1.0e06);

        return time;
    }

    double elapsedTimeInMs(quint32 frameID)
    {
        double time = 0;

        if (((frameID - m_frameID[m_readID]) < 2) || (m_readID == m_writeID))
            return time;

        if (m_absoluteTime) {
            quint64 startTime, endTime;

            m_timerStartQueryObjects[m_readID]->result(&startTime);
            m_timerEndQueryObjects[m_readID]->result(&endTime);

            m_averageTime[m_averageTimeWriteID] = endTime - startTime;
        } else {
            quint64 elapsedTime;

            m_timerStartQueryObjects[m_readID]->result(&elapsedTime);

            m_averageTime[m_averageTimeWriteID] = elapsedTime;
        }

        incrementReadCounter();
        incrementAveragedWriteCounter();

        return averagedElapsedTimeInMs();
    }
};


QSSGRenderGPUProfiler::QSSGRenderGPUProfiler(const QSSGRef<QSSGRenderContextInterface> &inContext, const QSSGRef<QSSGRenderContext> &inRenderContext)
    : m_renderContext(inRenderContext), m_context(inContext), m_vertexCount(0)
{
}

QSSGRenderGPUProfiler::~QSSGRenderGPUProfiler() { m_strToGpuTimerMap.clear(); }

void QSSGRenderGPUProfiler::startTimer(QString &nameID, bool absoluteTime, bool sync)
{
    QSSGRef<QSSGGpuTimerInfo> theGpuTimerData = getOrCreateGpuTimerInfo(nameID);

    if (theGpuTimerData) {
        if (sync)
            theGpuTimerData->addSync();

        theGpuTimerData->m_absoluteTime = absoluteTime;
        theGpuTimerData->startTimerQuery(m_context->frameCount());
    }
}

void QSSGRenderGPUProfiler::endTimer(QString &nameID)
{
    QSSGRef<QSSGGpuTimerInfo> theGpuTimerData = getOrCreateGpuTimerInfo(nameID);

    if (theGpuTimerData) {
        theGpuTimerData->endTimerQuery();
    }
}

double QSSGRenderGPUProfiler::elapsed(const QString &nameID) const
{
    double time = 0;
    QSSGRef<QSSGGpuTimerInfo> theGpuTimerData = getGpuTimerInfo(nameID);

    if (theGpuTimerData) {
        time = theGpuTimerData->elapsedTimeInMs(m_context->frameCount());
    }

    return time;
}

const QVector<QString> &QSSGRenderGPUProfiler::timerIDs() const { return m_timerIds; }

void QSSGRenderGPUProfiler::addVertexCount(quint32 count) { m_vertexCount += count; }

quint32 QSSGRenderGPUProfiler::getAndResetVertexCount() const
{
    quint32 v = m_vertexCount;
    m_vertexCount = 0;
    return v;
}

QSSGRef<QSSGGpuTimerInfo> QSSGRenderGPUProfiler::getOrCreateGpuTimerInfo(QString &nameID)
{
    TStrGpuTimerInfoMap::const_iterator theIter = m_strToGpuTimerMap.find(nameID);
    if (theIter != m_strToGpuTimerMap.end())
        return theIter.value();

    QSSGRef<QSSGGpuTimerInfo> theGpuTimerData = QSSGRef<QSSGGpuTimerInfo>(new QSSGGpuTimerInfo());

    if (theGpuTimerData) {
        // create queries
        for (quint32 i = 0; i < RECORDED_FRAME_DELAY; i++) {
            theGpuTimerData->m_timerStartQueryObjects[i] = QSSGRenderTimerQuery::create(m_renderContext);
            theGpuTimerData->m_timerEndQueryObjects[i] = QSSGRenderTimerQuery::create(m_renderContext);
            theGpuTimerData->m_timerSyncObjects[i] = QSSGRenderSync::create(m_renderContext);
            theGpuTimerData->m_frameID[i] = 0;
        }
        m_strToGpuTimerMap.insert(nameID, theGpuTimerData);
        m_timerIds.push_back(nameID);
    }

    return theGpuTimerData;
}

QSSGRef<QSSGGpuTimerInfo> QSSGRenderGPUProfiler::getGpuTimerInfo(const QString &nameID) const
{
    TStrGpuTimerInfoMap::const_iterator theIter = m_strToGpuTimerMap.find(nameID);
    if (theIter != m_strToGpuTimerMap.end())
        return theIter.value();

    return nullptr;
}

QT_END_NAMESPACE

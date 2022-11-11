// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquick3dprofileradapter.h"

#include <QCoreApplication>
#include <private/qqmldebugconnector_p.h>
#include <private/qversionedpacket_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>

QT_BEGIN_NAMESPACE

using QQmlDebugPacket = QVersionedPacket<QQmlDebugConnector>;

QQuick3DProfilerAdapter::QQuick3DProfilerAdapter(QObject *parent) :
    QQmlAbstractProfilerAdapter(parent), next(0)
{
    QQuick3DProfiler::initialize(this);
    // We can always do DirectConnection here as all methods are protected by mutexes
    connect(this, &QQmlAbstractProfilerAdapter::profilingEnabled,
            QQuick3DProfiler::s_instance, &QQuick3DProfiler::startProfilingImpl, Qt::DirectConnection);
    connect(this, &QQmlAbstractProfilerAdapter::profilingEnabledWhileWaiting,
            QQuick3DProfiler::s_instance, &QQuick3DProfiler::startProfilingImpl, Qt::DirectConnection);
    connect(this, &QQmlAbstractProfilerAdapter::referenceTimeKnown,
            QQuick3DProfiler::s_instance, &QQuick3DProfiler::setTimer, Qt::DirectConnection);
    connect(this, &QQmlAbstractProfilerAdapter::profilingDisabled,
            QQuick3DProfiler::s_instance, &QQuick3DProfiler::stopProfilingImpl, Qt::DirectConnection);
    connect(this, &QQmlAbstractProfilerAdapter::profilingDisabledWhileWaiting,
            QQuick3DProfiler::s_instance, &QQuick3DProfiler::stopProfilingImpl, Qt::DirectConnection);
    connect(this, &QQmlAbstractProfilerAdapter::dataRequested,
            QQuick3DProfiler::s_instance, &QQuick3DProfiler::reportDataImpl, Qt::DirectConnection);
    connect(QQuick3DProfiler::s_instance, &QQuick3DProfiler::dataReady,
            this, &QQuick3DProfilerAdapter::receiveData, Qt::DirectConnection);
}

QQuick3DProfilerAdapter::~QQuick3DProfilerAdapter()
{
    if (service)
        service->removeGlobalProfiler(this);
}


// convert to QByteArrays that can be sent to the debug client
static void QQuick3DProfilerDataToByteArrays(const QQuick3DProfilerData &data,
                                           QList<QByteArray> &messages)
{
    QQmlDebugPacket ds;
    Q_ASSERT_X(((data.messageType | data.detailType) & (1 << 31)) == 0, Q_FUNC_INFO,
               "You can use at most 31 message types and 31 detail types.");
    for (uint decodedMessageType = 0; (data.messageType >> decodedMessageType) != 0;
         ++decodedMessageType) {
        if ((data.messageType & (1 << decodedMessageType)) == 0)
            continue;

        for (uint decodedDetailType = 0; (data.detailType >> decodedDetailType) != 0;
             ++decodedDetailType) {
            if ((data.detailType & (1 << decodedDetailType)) == 0)
                continue;

            ds << data.time << decodedMessageType << decodedDetailType;
            switch (decodedMessageType) {
            case QQuick3DProfiler::Event:
                break;
            case QQuick3DProfiler::Quick3DFrame:
                switch (decodedDetailType) {
                case QQuick3DProfiler::Quick3DSynchronizeFrame:
                case QQuick3DProfiler::Quick3DPrepareFrame:
                case QQuick3DProfiler::Quick3DLoadShader:
                case QQuick3DProfiler::Quick3DGenerateShader:
                    ds << data.subdata1;
                    break;
                case QQuick3DProfiler::Quick3DRenderFrame:
                case QQuick3DProfiler::Quick3DMeshLoad:
                case QQuick3DProfiler::Quick3DCustomMeshLoad:
                case QQuick3DProfiler::Quick3DTextureLoad:
                case QQuick3DProfiler::Quick3DParticleUpdate:
                    ds << data.subdata1 << data.subdata2;
                    break;
                default:
                    Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid decoded detail type");
                }
                break;
            default:
                Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid message type.");
                break;

            }

            messages.append(ds.squeezedData());
            ds.clear();
        }
    }
}

qint64 QQuick3DProfilerAdapter::sendMessages(qint64 until, QList<QByteArray> &messages)
{
    while (next < m_data.size()) {
        if (m_data[next].time <= until && messages.size() <= s_numMessagesPerBatch)
            QQuick3DProfilerDataToByteArrays(m_data[next++], messages);
        else
            return m_data[next].time;
    }
    m_data.clear();
    next = 0;
    return -1;
}

void QQuick3DProfilerAdapter::receiveData(const QVector<QQuick3DProfilerData> &new_data)
{
    if (m_data.isEmpty())
        m_data = new_data;
    else
        m_data.append(new_data);
    service->dataReady(this);
}

QT_END_NAMESPACE

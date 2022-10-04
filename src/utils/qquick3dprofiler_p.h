// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICK3DPROFILER_P_H
#define QQUICK3DPROFILER_P_H

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

#include <QtCore/private/qabstractanimation_p.h>
#include <QtQuick/private/qtquickglobal_p.h>
#include <QtQuick3DUtils/private/qtquick3dutilsglobal_p.h>


#if QT_CONFIG(qml_debug)
#include <QtQml/private/qqmlprofilerdefinitions_p.h>
#endif

#include <QtCore/qurl.h>
#include <QtCore/qsize.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthreadstorage.h>

QT_BEGIN_NAMESPACE

#if !QT_CONFIG(qml_debug)

#define Q_QUICK3D_PROFILE_IF_ENABLED(feature, Code)

struct QQuick3DProfiler {

};

#define Q_QUICK3D_PROFILING_ENABLED false

#else

#define Q_QUICK3D_PROFILE_IF_ENABLED(feature, Code)\
    if (QQuick3DProfiler::featuresEnabled & (1 << feature)) {\
        Code;\
    } else\
        (void)0

#define Q_QUICK3D_PROFILING_ENABLED (QQuick3DProfiler::featuresEnabled > 0)

// This struct is somewhat dangerous to use:
// You can save values either with 32 or 64 bit precision. toByteArrays will
// guess the precision from messageType. If you state the wrong messageType
// you will get undefined results.
// The messageType is itself a bit field. You can pack multiple messages into
// one object, e.g. RangeStart and RangeLocation. Each one will be read
// independently by toByteArrays. Thus you can only pack messages if their data
// doesn't overlap. Again, it's up to you to figure that out.
struct Q_AUTOTEST_EXPORT QQuick3DProfilerData
{
    QQuick3DProfilerData() {}

    QQuick3DProfilerData(qint64 time, int messageType, int detailType, qint64 d1, qint64 d2) :
        time(time), messageType(messageType), detailType(detailType), subdata1(d1), subdata2(d2) {}

    qint64 time;
    int messageType;
    int detailType;
    qint64 subdata1;
    qint64 subdata2;
};

Q_DECLARE_TYPEINFO(QQuick3DProfilerData, Q_RELOCATABLE_TYPE);

class QQuick3DProfilerSceneGraphData : public QQmlProfilerDefinitions {
private:
    static const uint s_numSceneGraphTimings = 2;
    static const uint s_numNestedTimings = 5;
    struct Timings {
        uint nesting = 0;
        qint64 values[s_numNestedTimings][s_numSceneGraphTimings + 1];
    };
    template<uint size>
    struct TimingData
    {
        Timings timings[size];
    };
    QThreadStorage<TimingData<MaximumQuick3DFrameType>> eventTimings;
public:
    template<int type, bool inc>
    qint64 *timings()
    {
        Timings *timings;
        qint64 *t;
        timings = &eventTimings.localData().timings[type];
        if (inc) {
            Q_ASSERT(timings->nesting < s_numNestedTimings);
            t = timings->values[timings->nesting];
            timings->nesting++;
        } else {
            timings->nesting--;
            t = timings->values[timings->nesting];
        }
        return t;
    }
};

class Q_QUICK3DUTILS_EXPORT QQuick3DProfiler : public QObject, public QQmlProfilerDefinitions {
    Q_OBJECT
public:

    enum Quick3DStage {
        Quick3DStageBegin,
        Quick3DStageEnd
    };

    template<Quick3DFrameType FrameType>
    static void recordSceneGraphTimestamp(uint position)
    {
        s_instance->m_sceneGraphData.timings<FrameType, true>()[position] = s_instance->timestamp();
    }
    template<Quick3DFrameType FrameType, bool Record>
    static void reportQuick3DFrame(uint position, quint64 payload = ~0)
    {
        qint64 *timings = s_instance->m_sceneGraphData.timings<FrameType, false>();
        if (Record)
            timings[position] = s_instance->timestamp();
        s_instance->processMessage(QQuick3DProfilerData(
                timings[position], Quick3DFrame, FrameType,
                position > 0 ? timings[1] - timings[0] : payload,
                position > 1 ? timings[2] - timings[1] : payload));
    }

    template<Quick3DFrameType FrameType, bool Record, Quick3DFrameType SwitchTo>
    static void reportQuick3DFrame(uint position, quint64 payload = ~0)
    {
        reportQuick3DFrame<FrameType, Record>(position, payload);
        s_instance->m_sceneGraphData.timings<SwitchTo, false>()[0] =
                s_instance->m_sceneGraphData.timings<FrameType>()[position];
    }

    qint64 timestamp() { return m_timer.nsecsElapsed(); }

    static quint64 featuresEnabled;

    static void initialize(QObject *parent);

    ~QQuick3DProfiler() override;

signals:
    void dataReady(const QVector<QQuick3DProfilerData> &data);

protected:
    friend class QQuick3DProfilerAdapter;

    static QQuick3DProfiler *s_instance;
    QMutex m_dataMutex;
    QElapsedTimer m_timer;
    QVector<QQuick3DProfilerData> m_data;
    QQuick3DProfilerSceneGraphData m_sceneGraphData;

    QQuick3DProfiler(QObject *parent);

    void processMessage(const QQuick3DProfilerData &message)
    {
        QMutexLocker lock(&m_dataMutex);
        m_data.append(message);
    }

    void startProfilingImpl(quint64 features);
    void stopProfilingImpl();
    void reportDataImpl();
    void setTimer(const QElapsedTimer &t);
};

#endif // QT_CONFIG(qml_debug)

#define Q_QUICK3D_PROFILE(feature, Method) \
    Q_QUICK3D_PROFILE_IF_ENABLED(feature, QQuick3DProfiler::Method)

#define Q_QUICK3D_PROFILE_START(Type) \
    Q_QUICK3D_PROFILE_IF_ENABLED(QQuick3DProfiler::ProfileQuick3D, \
                               (QQuick3DProfiler::recordSceneGraphTimestamp<Type>(QQuick3DProfiler::Quick3DStageBegin)))

// report \a Type, using data points 0 to \a position, including \a position, and setting the
// timestamp at \a position to the current one.
#define Q_QUICK3D_PROFILE_END(Type) \
    Q_QUICK3D_PROFILE_IF_ENABLED(QQuick3DProfiler::ProfileQuick3D, \
                               (QQuick3DProfiler::reportQuick3DFrame<Type, true>(QQuick3DProfiler::Quick3DStageEnd)))

// report \a Type, using data points 0 to \a position, including \a position, and setting the
// timestamp at \a position to the current one. Remaining data points up to position 5 are filled
// with \a Payload.
#define Q_QUICK3D_PROFILE_END_WITH_PAYLOAD(Type, Payload) \
    Q_QUICK3D_PROFILE_IF_ENABLED(QQuick3DProfiler::ProfileQuick3D, \
                               (QQuick3DProfiler::reportQuick3DFrame<Type, true>(QQuick3DProfiler::Quick3DStageEnd, \
                                                                                  Payload)))

QT_END_NAMESPACE

#endif

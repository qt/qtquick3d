/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
        time(time), messageType(messageType), detailType(detailType), subtime_1(d1), subtime_2(d2) {}

    qint64 time;
    int messageType;
    int detailType;
    qint64 subtime_1;
    qint64 subtime_2;
};

Q_DECLARE_TYPEINFO(QQuick3DProfilerData, Q_RELOCATABLE_TYPE);

class QQuick3DProfilerSceneGraphData : public QQmlProfilerDefinitions {
private:
    static const uint s_numSceneGraphTimings = 2;

    template<uint size>
    struct TimingData {
        qint64 values[size][s_numSceneGraphTimings + 1];
    };

    QThreadStorage<TimingData<NumQuick3DRenderThreadFrameTypes> > renderThreadTimings;
    TimingData<NumQuick3DGUIThreadFrameTypes> guiThreadTimings;

public:
    template<Quick3DFrameType type>
    qint64 *timings()
    {
        if (type < NumQuick3DRenderThreadFrameTypes)
            return renderThreadTimings.localData().values[type];
        else
            return guiThreadTimings.values[type - NumQuick3DRenderThreadFrameTypes];
    }
};

class Q_QUICK3DUTILS_EXPORT QQuick3DProfiler : public QObject, public QQmlProfilerDefinitions {
    Q_OBJECT
public:

    enum Quick3DStage {
        Quick3DStageBegin,
        Quick3DStageEnd
    };

    template<Quick3DFrameType FrameType1, Quick3DFrameType FrameType2>
    static void startQuick3DFrame()
    {
        startQuick3DFrame<FrameType1>();
        s_instance->m_sceneGraphData.timings<FrameType2>()[0] =
                s_instance->m_sceneGraphData.timings<FrameType1>()[0];
    }

    template<Quick3DFrameType FrameType>
    static void startQuick3DFrame()
    {
        s_instance->m_sceneGraphData.timings<FrameType>()[0] = s_instance->timestamp();
    }

    template<Quick3DFrameType FrameType>
    static void recordSceneGraphTimestamp(uint position)
    {
        s_instance->m_sceneGraphData.timings<FrameType>()[position] = s_instance->timestamp();
    }

    template<Quick3DFrameType FrameType, uint Skip>
    static void skipSceneGraphTimestamps(uint position)
    {
        qint64 *timings = s_instance->m_sceneGraphData.timings<FrameType>();
        const qint64 last = timings[position];
        for (uint i = 0; i < Skip; ++i)
            timings[++position] = last;
    }

    template<Quick3DFrameType FrameType, bool Record>
    static void reportQuick3DFrame(uint position, quint64 payload = ~0)
    {
        qint64 *timings = s_instance->m_sceneGraphData.timings<FrameType>();
        if (Record)
            timings[position] = s_instance->timestamp();
        s_instance->processMessage(QQuick3DProfilerData(
                timings[position], 1 << Quick3DFrame, 1 << FrameType,
                position > 0 ? timings[1] - timings[0] : payload,
                position > 1 ? timings[2] - timings[1] : payload));
    }

    template<Quick3DFrameType FrameType, bool Record, Quick3DFrameType SwitchTo>
    static void reportQuick3DFrame(uint position, quint64 payload = ~0)
    {
        reportQuick3DFrame<FrameType, Record>(position, payload);
        s_instance->m_sceneGraphData.timings<SwitchTo>()[0] =
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

#define Q_QUICK3D_PROFILE(feature, Method)\
    Q_QUICK3D_PROFILE_IF_ENABLED(feature, QQuick3DProfiler::Method)

// Record current timestamp for \a Type at position 0.
#define Q_QUICK3D_PROFILE_START(Type)\
    Q_QUICK3D_PROFILE_IF_ENABLED(QQuick3DProfiler::ProfileQuick3D,\
                               (QQuick3DProfiler::startQuick3DFrame<Type>()))

// Record current timestamp for \a Type at \a position.
#define Q_QUICK3D_PROFILE_RECORD(Type, position)\
    Q_QUICK3D_PROFILE_IF_ENABLED(QQuick3DProfiler::ProfileQuick3D,\
                               (QQuick3DProfiler::recordSceneGraphTimestamp<Type>(position)))

// Use the timestamp for \a Type at position \a position and repeat it \a Skip times in subsequent
// positions.
#define Q_QUICK3D_PROFILE_SKIP(Type, position, Skip)\
    Q_QUICK3D_PROFILE_IF_ENABLED(QQuick3DProfiler::ProfileQuick3D,\
                               (QQuick3DProfiler::skipSceneGraphTimestamps<Type, Skip>(position)))

// Record current timestamp for both \a Type1 and \a Type2 at position 0.
#define Q_QUICK3D_PROFILE_START_SYNCHRONIZED(Type1, Type2)\
    Q_QUICK3D_PROFILE_IF_ENABLED(QQuick3DProfiler::ProfileQuick3D,\
                               (QQuick3DProfiler::startQuick3DFrame<Type1, Type2>()))

// report \a Type1, using the current timestamp at \a position, and switch to \a Typ2, using
// the current timestamp at position 0.
#define Q_QUICK3D_PROFILE_SWITCH(Type1, Type2, position)\
    Q_QUICK3D_PROFILE_IF_ENABLED(QQuick3DProfiler::ProfileQuick3D,\
                               (QQuick3DProfiler::reportQuick3DFrame<Type1, true, Type2>(\
                                    position)))

// report \a Type, using data points 0 to \a position, including \a position.
#define Q_QUICK3D_PROFILE_REPORT(Type, position)\
    Q_QUICK3D_PROFILE_IF_ENABLED(QQuick3DProfiler::ProfileQuick3D,\
                               (QQuick3DProfiler::reportQuick3DFrame<Type, false>(position)))

// report \a Type, using data points 0 to \a position, including \a position, and setting the
// timestamp at \a position to the current one.
#define Q_QUICK3D_PROFILE_END(Type, position)\
    Q_QUICK3D_PROFILE_IF_ENABLED(QQuick3DProfiler::ProfileQuick3D,\
                               (QQuick3DProfiler::reportQuick3DFrame<Type, true>(position)))

// report \a Type, using data points 0 to \a position, including \a position, and setting the
// timestamp at \a position to the current one. Remaining data points up to position 5 are filled
// with \a Payload.
#define Q_QUICK3D_PROFILE_END_WITH_PAYLOAD(Type, position, Payload)\
    Q_QUICK3D_PROFILE_IF_ENABLED(QQuick3DProfiler::ProfileQuick3D,\
                               (QQuick3DProfiler::reportQuick3DFrame<Type, true>(position,\
                                                                                  Payload)))

QT_END_NAMESPACE

#endif

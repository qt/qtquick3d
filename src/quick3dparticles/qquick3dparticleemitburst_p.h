/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QQUICK3DPARTICLEEMITBURST_H
#define QQUICK3DPARTICLEEMITBURST_H

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

#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlparserstatus.h>
#include <QtQuick3DParticles/qtquick3dparticlesglobal.h>

QT_BEGIN_NAMESPACE

class QQuick3DParticleEmitter;

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleEmitBurst : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(int time READ time WRITE setTime NOTIFY timeChanged)
    Q_PROPERTY(int amount READ amount WRITE setAmount NOTIFY amountChanged)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(bool repeat READ repeat WRITE setRepeat NOTIFY repeatChanged REVISION(6, 3))
    Q_PROPERTY(int repeatDelay READ repeatDelay WRITE setRepeatDelay NOTIFY repeatDelayChanged REVISION(6, 3))
    Q_PROPERTY(int amountVariation READ amountVariation WRITE setAmountVariation NOTIFY amountVariationChanged REVISION(6, 3))
    Q_PROPERTY(bool startTrigger READ startTrigger WRITE setStartTrigger NOTIFY startTriggerChanged REVISION(6, 3))
    Q_PROPERTY(bool endTrigger READ endTrigger WRITE setEndTrigger NOTIFY endTriggerChanged REVISION(6, 3))
    Q_PROPERTY(bool triggerOnly READ triggerOnly WRITE setTriggerOnly NOTIFY triggerOnlyChanged REVISION(6, 3))

    QML_NAMED_ELEMENT(EmitBurst3D)
    Q_INTERFACES(QQmlParserStatus)
    QML_ADDED_IN_VERSION(6, 2)

public:
    QQuick3DParticleEmitBurst(QObject *parent = nullptr);
    ~QQuick3DParticleEmitBurst() override;

    int time() const;
    int amount() const;
    int duration() const;
    bool repeat() const;
    int repeatDelay() const;
    int repeatStartTime() const { return m_repeatStartTime; }
    int amountVariation() const;
    bool startTrigger() const;
    bool endTrigger() const;
    bool triggerOnly() const;
public Q_SLOTS:
    void setTime(int time);
    void setAmount(int amount);
    void setDuration(int duration);
    void setRepeat(bool repeat);
    void setRepeatDelay(int delay);
    void setRepeatStartTime(int time) { m_repeatStartTime = time; }
    void setAmountVariation(int value);
    void setEndTrigger(bool value);
    void setStartTrigger(bool value);
    void setTriggerOnly(bool value);

Q_SIGNALS:
    void timeChanged();
    void amountChanged();
    void durationChanged();
    Q_REVISION(6, 3) void repeatChanged();
    Q_REVISION(6, 3) void repeatDelayChanged();
    Q_REVISION(6, 3) void amountVariationChanged();
    Q_REVISION(6, 3) void endTriggerChanged();
    Q_REVISION(6, 3) void startTriggerChanged();
    Q_REVISION(6, 3) void triggerOnlyChanged();

protected:
    // From QQmlParserStatus
    void componentComplete() override;
    void classBegin() override {}

private:
    friend class QQuick3DParticleEmitter;
    friend class QQuick3DParticleSystem;

    enum TriggerType
    {
        TriggerNone = 0,
        TriggerEmit = 1,
        TriggerStart = 2,
        TriggerEnd = 4
    };

    struct BurstEmitData
    {
        int startTime;
        int endTime;
        int aggregateTime;
        int emitAmount;
        int emitCounter;
        int emitTimePerParticle;
    };

    QQuick3DParticleEmitter *m_parentEmitter = nullptr;
    int m_time = 0;
    int m_amount = 0;
    int m_duration = 0;

    int m_repeatDelay = 0;
    int m_repeatStartTime = 0;
    int m_amountVariation = 0;

    bool m_repeat = false;
    TriggerType m_triggerType = TriggerEmit;
    QList<BurstEmitData> m_burstEmitData;
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLEEMITBURST_H

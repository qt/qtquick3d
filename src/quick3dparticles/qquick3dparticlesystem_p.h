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

#ifndef QQUICK3DPPARTICLESYSTEM_H
#define QQUICK3DPPARTICLESYSTEM_H

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

#include <QtQuick3DParticles/qtquick3dparticlesglobal.h>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3ddefaultmaterial_p.h>
#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3dloader_p.h>
#include <QtQuick3DParticles/private/qquick3dparticlesystemlogging_p.h>
#include <QtQuick3DParticles/private/qquick3dparticlerandomizer_p.h>
#include <QElapsedTimer>
#include <QVector>
#include <QList>
#include <QHash>
#include <QPointer>
#include <QAbstractAnimation>
#include <QtQml/qqml.h>
#include <QElapsedTimer>
#include <QTimer>

QT_BEGIN_NAMESPACE

class QQuick3DParticleSpriteParticle;
class QQuick3DParticleModelParticle;
class QQuick3DParticleEmitter;
class QQuick3DParticleTrailEmitter;
class QQuick3DParticleAffector;
class QQuick3DParticleStatelessAffector;
struct QQuick3DParticleData;

class QQuick3DParticle;
class QQuick3DParticleSystemAnimation;
class QQuick3DParticleSystemUpdate;
class QQuick3DParticleInstanceTable;

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleSystem : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(int startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged)
    Q_PROPERTY(int time READ time WRITE setTime NOTIFY timeChanged)
    Q_PROPERTY(bool useRandomSeed READ useRandomSeed WRITE setUseRandomSeed NOTIFY useRandomSeedChanged)
    Q_PROPERTY(int seed READ seed WRITE setSeed NOTIFY seedChanged)
    Q_PROPERTY(bool logging READ logging WRITE setLogging NOTIFY loggingChanged)
    Q_PROPERTY(QQuick3DParticleSystemLogging *loggingData READ loggingData NOTIFY loggingDataChanged)
    QML_NAMED_ELEMENT(ParticleSystem3D)
    QML_ADDED_IN_VERSION(6, 1)

public:
    QQuick3DParticleSystem(QQuick3DNode *parent = nullptr);
    ~QQuick3DParticleSystem() override;

    bool isRunning() const;
    bool isPaused() const;
    int startTime() const;
    int time() const;
    bool useRandomSeed() const;
    int seed() const;
    // Return the total amount of particles
    int particleCount() const;
    bool logging() const;
    QQuick3DParticleSystemLogging *loggingData() const;

    // Registering of different components into system
    void registerParticle(QQuick3DParticle *particle);
    void unRegisterParticle(QQuick3DParticle *particle);
    void registerParticleEmitter(QQuick3DParticleEmitter* e);
    void unRegisterParticleEmitter(QQuick3DParticleEmitter* e);
    void registerParticleAffector(QQuick3DParticleAffector* a);
    void unRegisterParticleAffector(QQuick3DParticleAffector* a);

    void updateCurrentTime(int currentTime);

    QPRand *rand();

public Q_SLOTS:
    void setRunning(bool arg);
    void setPaused(bool arg);
    void setStartTime(int startTime);
    void setTime(int time);
    void setUseRandomSeed(bool randomize);
    void setSeed(int seed);
    void setLogging(bool logging);

private Q_SLOTS:
    void refresh();
    void markDirty();

protected:
    void componentComplete() override;

Q_SIGNALS:
    void runningChanged();
    void pausedChanged();
    void timeChanged();
    void startTimeChanged();
    void useRandomSeedChanged();
    void seedChanged();
    void loggingChanged();
    void loggingDataChanged();

private:
    void registerParticleModel(QQuick3DParticleModelParticle* m);
    void registerParticleSprite(QQuick3DParticleSpriteParticle* m);
    void reset();
    void updateLoggingData();
    void resetLoggingVariables();
    void doSeedRandomization();

private:
    friend class QQuick3DParticleEmitter;
    friend class QQuick3DParticleTrailEmitter;
    friend class QQuick3DParticleSystemUpdate;

    bool m_running;
    bool m_paused;
    bool m_initialized;
    bool m_componentComplete;
    // This animation runs the system, progressing time with pause, continue etc.
    QQuick3DParticleSystemAnimation* m_animation = nullptr;
    // This animation handles system dirty updates and runs always.
    // It makes sure that updates are done in sync with other animations and only once per frame.
    QQuick3DParticleSystemUpdate *m_updateAnimation = nullptr;

    QList<QQuick3DParticleModelParticle *> m_modelParticles;
    QList<QQuick3DParticleSpriteParticle *> m_spriteParticles;
    QList<QQuick3DParticle *> m_particles;
    QList<QQuick3DParticleEmitter *> m_emitters;
    QList<QQuick3DParticleTrailEmitter *> m_trailEmitters;
    QList<QQuick3DParticleAffector *> m_affectors;
    QMap<QQuick3DParticleAffector *, QMetaObject::Connection> m_connections;

    int m_startTime = 0;
    // Current time in ms
    int m_time = 0;
    QElapsedTimer m_perfTimer;
    QTimer m_loggingTimer;
    qint64 m_timeAnimation = 0;
    int m_particlesMax = 0;
    int m_particlesUsed = 0;
    int m_updates = 0;
    bool m_useRandomSeed = true;
    int m_seed = 0;
    bool m_logging;
    QQuick3DParticleSystemLogging *m_loggingData = nullptr;
    QPRand m_rand;

    struct TrailEmits {
        QQuick3DParticleTrailEmitter *emitter = nullptr;
        int amount = 0;
    };

};

class QQuick3DParticleSystemAnimation : public QAbstractAnimation
{
    Q_OBJECT
public:
    QQuick3DParticleSystemAnimation(QQuick3DParticleSystem* system)
        : QAbstractAnimation(static_cast<QObject*>(system)), m_system(system)
    { }
protected:
    void updateCurrentTime(int t) override
    {
        m_system->updateCurrentTime(t + m_system->startTime());
    }

    int duration() const override
    {
        return -1;
    }

private:
    QQuick3DParticleSystem* m_system;
};

class QQuick3DParticleSystemUpdate : public QAbstractAnimation
{
    Q_OBJECT
public:
    QQuick3DParticleSystemUpdate(QQuick3DParticleSystem* system)
        : QAbstractAnimation(static_cast<QObject*>(system)), m_system(system)
    { }

    void setDirty(bool dirty)
    {
        m_dirty = dirty;
    }

protected:
    void updateCurrentTime(int t) override
    {
        Q_UNUSED(t);
        if (m_dirty)
            m_system->refresh();
    }

    int duration() const override
    {
        return -1;
    }

private:
    QQuick3DParticleSystem* m_system;
    bool m_dirty = false;
};

QT_END_NAMESPACE

#endif // QQUICK3DPPARTICLESYSTEM_H



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

#ifndef QQUICK3DPARTICLEEMITTER_H
#define QQUICK3DPARTICLEEMITTER_H

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

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3DParticles/private/qquick3dparticledirection_p.h>
#include <QtQuick3DParticles/private/qquick3dparticlesystem_p.h>
#include <QtQuick3DParticles/private/qquick3dparticlemodelparticle_p.h>
#include <QtQuick3DParticles/private/qquick3dparticleshape_p.h>
#include <QtQuick3DParticles/private/qquick3dparticleemitburst_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleEmitter : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DParticleSystem* system READ system WRITE setSystem NOTIFY systemChanged)
    Q_PROPERTY(QQmlListProperty<QQuick3DParticleEmitBurst> emitBursts READ emitBursts)
    Q_PROPERTY(QQuick3DParticleDirection *velocity READ velocity WRITE setVelocity NOTIFY velocityChanged)
    // Note: Currently each emitter can have only one particle
    Q_PROPERTY(QQuick3DParticle *particle READ particle WRITE setParticle NOTIFY particleChanged)
    // When particle is not enabled, it doesn't emit. Not even bursts.
    // Default value true
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    // Shape where particles are emitted inside. Shape is scaled, positioned and rotated based on
    // emitter node. When Shape fill property is set to false, emitting happens only from outsides of the shape.
    // By default, the shape is not set and emitting is done from center point of the emitter node.
    Q_PROPERTY(QQuick3DParticleShape *shape READ shape WRITE setShape NOTIFY shapeChanged)
    // Emitting amount in particles per second.
    Q_PROPERTY(int emitRate READ emitRate WRITE setEmitRate NOTIFY emitRateChanged)
    Q_PROPERTY(int lifeSpan READ lifeSpan WRITE setLifeSpan NOTIFY lifeSpanChanged)
    Q_PROPERTY(int lifeSpanVariation READ lifeSpanVariation WRITE setLifeSpanVariation NOTIFY lifeSpanVariationChanged)

    // Multiplier for the particle scale in the beginning.
    Q_PROPERTY(float particleScale READ particleScale WRITE setParticleScale NOTIFY particleScaleChanged)
    // Default value -1, meaning endScale will match to the beginning scale.
    Q_PROPERTY(float particleEndScale READ particleEndScale WRITE setParticleEndScale NOTIFY particleEndScaleChanged)
    // Affects both start and end scales, own properties?
    Q_PROPERTY(float particleScaleVariation READ particleScaleVariation WRITE setParticleScaleVariation NOTIFY particleScaleVariationChanged)

    // In degrees
    Q_PROPERTY(QVector3D particleRotation READ particleRotation WRITE setParticleRotation NOTIFY particleRotationChanged)
    // In degrees
    Q_PROPERTY(QVector3D particleRotationVariation READ particleRotationVariation WRITE setParticleRotationVariation NOTIFY particleRotationVariationChanged)
    // In degrees per second
    Q_PROPERTY(QVector3D particleRotationVelocity READ particleRotationVelocity WRITE setParticleRotationVelocity NOTIFY particleRotationVelocityChanged)
    // In degrees per second
    Q_PROPERTY(QVector3D particleRotationVelocityVariation READ particleRotationVelocityVariation WRITE setParticleRotationVelocityVariation NOTIFY particleRotationVariationVelocityChanged)
    QML_NAMED_ELEMENT(ParticleEmitter3D)

public:
    QQuick3DParticleEmitter(QQuick3DNode *parent = nullptr);
    ~QQuick3DParticleEmitter() override;

    bool enabled() const;
    QQuick3DParticleDirection* velocity() const;
    QQuick3DParticleSystem* system() const;
    int emitRate() const;
    float particleScale() const;
    float particleEndScale() const;
    float particleScaleVariation() const;
    int lifeSpan() const;
    int lifeSpanVariation() const;
    QQuick3DParticle *particle() const;
    QQuick3DParticleShape * shape() const;
    // Rotations in degrees of euler rotation
    QVector3D particleRotation() const;
    QVector3D particleRotationVariation() const;
    QVector3D particleRotationVelocity() const;
    QVector3D particleRotationVelocityVariation() const;

    // TODO: Proper API documentation
    // Emit \a count amount of particles immediately
    Q_INVOKABLE virtual void burst(int count);
    // Emit \a count amount of particles, distributed during \a duration.
    Q_INVOKABLE virtual void burst(int count, int duration);
    // Emit \a count amount of particles, distributed during \a duration.
    // The particles are emitted as if the Emitter was at \a position but all other properties are the same.
    Q_INVOKABLE virtual void burst(int count, int duration, const QVector3D &position);

public Q_SLOTS:
    void setEnabled(bool enabled);
    void setVelocity(QQuick3DParticleDirection *velocity);
    void setSystem(QQuick3DParticleSystem* system);
    void setEmitRate(int emitRate);
    void setParticleScale(float particleScale);
    void setParticleEndScale(float particleEndScale);
    void setParticleScaleVariation(float particleScaleVariation);
    void setLifeSpan(int lifeSpan);
    void setLifeSpanVariation(int lifeSpanVariation);
    void setParticle(QQuick3DParticle *particle);
    void setShape(QQuick3DParticleShape * shape);
    void setParticleRotation(const QVector3D &particleRotation);
    void setParticleRotationVariation(const QVector3D &particleRotationVariation);
    void setParticleRotationVelocity(const QVector3D &particleRotationVelocity);
    void setParticleRotationVelocityVariation(const QVector3D &particleRotationVelocityVariation);

protected:
    void componentComplete() override;

Q_SIGNALS:
    void velocityChanged();
    void systemChanged();
    void emitRateChanged();
    void particleScaleChanged();
    void particleEndScaleChanged();
    void particleScaleVariationChanged();
    void lifeSpanChanged();
    void lifeSpanVariationChanged();
    void particleChanged();
    void shapeChanged();
    void particleRotationChanged();
    void particleRotationVariationChanged();
    void particleRotationVelocityChanged();
    void particleRotationVariationVelocityChanged();
    void enabledChanged();

protected:
    friend class QQuick3DParticleSystem;
    friend class QQuick3DParticleEmitBurst;

    void registerEmitBurst(QQuick3DParticleEmitBurst* emitBurst);
    void unRegisterEmitBurst(QQuick3DParticleEmitBurst* emitBurst);
    void generateEmitBursts();
    void emitParticle(QQuick3DParticle *particle, float startTime, const QVector3D &centerPos);
    void emitParticles();
    void emitParticlesBurst(const QQuick3DParticleEmitBurstData &burst);
    int getEmitAmount();

    void reset();

    QQuick3DParticleDirection *m_velocity = nullptr;
    QQuick3DParticleSystem* m_system = nullptr;
    int m_emitRate = 0;
    // Time in ms when emitting last time happened
    int m_prevEmitTime = 0;
    float m_particleScale = 1.0;
    float m_particleEndScale = -1.0;
    float m_particleScaleVariation = 0.0;
    int m_lifeSpan = 1000;
    int m_lifeSpanVariation = 0;
    float m_unemittedF = 0;
    QQuick3DParticle *m_particle = nullptr;
    QQuick3DParticleShape *m_shape = nullptr;
    QVector3D m_particleRotation;
    QVector3D m_particleRotationVariation;
    QVector3D m_particleRotationVelocity;
    QVector3D m_particleRotationVelocityVariation;
    bool m_enabled = true;
    const QQuick3DParticleData m_clearData = {};
    bool m_burstGenerated = false;

protected:
    // EmitBursts - list handling
    QQmlListProperty<QQuick3DParticleEmitBurst> emitBursts();
    void appendEmitBurst(QQuick3DParticleEmitBurst*);
    qsizetype emitBurstCount() const;
    QQuick3DParticleEmitBurst *emitBurst(qsizetype) const;
    void clearEmitBursts();
    void replaceEmitBurst(qsizetype, QQuick3DParticleEmitBurst*);
    void removeLastEmitBurst();

    // EmitBursts - static
    static void appendEmitBurst(QQmlListProperty<QQuick3DParticleEmitBurst>*, QQuick3DParticleEmitBurst*);
    static qsizetype emitBurstCount(QQmlListProperty<QQuick3DParticleEmitBurst>*);
    static QQuick3DParticleEmitBurst* emitBurst(QQmlListProperty<QQuick3DParticleEmitBurst>*, qsizetype);
    static void clearEmitBursts(QQmlListProperty<QQuick3DParticleEmitBurst>*);
    static void replaceEmitBurst(QQmlListProperty<QQuick3DParticleEmitBurst>*, qsizetype, QQuick3DParticleEmitBurst*);
    static void removeLastEmitBurst(QQmlListProperty<QQuick3DParticleEmitBurst>*);
    QList<QQuick3DParticleEmitBurst *> m_emitBursts;
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLEEMITTER_H

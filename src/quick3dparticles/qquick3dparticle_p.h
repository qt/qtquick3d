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

#ifndef QQUICK3DPARTICLE_H
#define QQUICK3DPARTICLE_H

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

#include <QColor>
#include <QVector4D>

#include <QtQuick3DParticles/private/qquick3dparticlesystem_p.h>
#include <QtQuick3DParticles/private/qquick3dparticledata_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticle : public QQuick3DObject
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DParticleSystem* system READ system WRITE setSystem NOTIFY systemChanged)
    Q_PROPERTY(int maxAmount READ maxAmount WRITE setMaxAmount NOTIFY maxAmountChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged RESET resetColor)
    Q_PROPERTY(QVector4D colorVariation READ colorVariation WRITE setColorVariation NOTIFY colorVariationChanged)
    Q_PROPERTY(bool unifiedColorVariation READ unifiedColorVariation WRITE setUnifiedColorVariation NOTIFY unifiedColorVariationChanged)
    Q_PROPERTY(FadeType fadeInEffect READ fadeInEffect WRITE setFadeInEffect NOTIFY fadeInEffectChanged)
    Q_PROPERTY(FadeType fadeOutEffect READ fadeOutEffect WRITE setFadeOutEffect NOTIFY fadeOutEffectChanged)
    Q_PROPERTY(int fadeInDuration READ fadeInDuration WRITE setFadeInDuration NOTIFY fadeInDurationChanged)
    Q_PROPERTY(int fadeOutDuration READ fadeOutDuration WRITE setFadeOutDuration NOTIFY fadeOutDurationChanged)
    Q_PROPERTY(AlignMode alignMode READ alignMode WRITE setAlignMode NOTIFY alignModeChanged)
    Q_PROPERTY(QVector3D alignTargetPosition READ alignTargetPosition WRITE setAlignTargetPosition NOTIFY alignTargetPositionChanged)
    QML_NAMED_ELEMENT(Particle3D)
    QML_UNCREATABLE("Particle3D is abstract")
    QML_ADDED_IN_VERSION(6, 1)

public:
    QQuick3DParticle(QQuick3DObject *parent = nullptr);
    ~QQuick3DParticle() override;

    enum FadeType
    {
        FadeNone,
        FadeOpacity,
        FadeScale
    };
    Q_ENUM(FadeType)

    enum AlignMode
    {
        AlignNone,
        AlignTowardsTarget,
        AlignTowardsStartVelocity
    };
    Q_ENUM(AlignMode)

    QQuick3DParticleSystem* system() const;
    int maxAmount() const;
    QColor color() const;
    QVector4D colorVariation() const;
    bool unifiedColorVariation() const;
    FadeType fadeInEffect() const;
    FadeType fadeOutEffect() const;
    int fadeInDuration() const;
    int fadeOutDuration() const;
    AlignMode alignMode() const;
    QVector3D alignTargetPosition() const;

    float opacity() const;
    void resetColor();

public Q_SLOTS:
    void setSystem(QQuick3DParticleSystem* system);
    void setMaxAmount(int maxAmount);
    void setColor(QColor color);
    void setColorVariation(QVector4D colorVariation);
    void setUnifiedColorVariation(bool unified);
    void setFadeInEffect(QQuick3DParticle::FadeType fadeInEffect);
    void setFadeOutEffect(QQuick3DParticle::FadeType fadeOutEffect);
    void setFadeInDuration(int fadeInDuration);
    void setFadeOutDuration(int fadeOutDuration);
    void setAlignMode(AlignMode alignMode);
    void setAlignTargetPosition(const QVector3D &alignPosition);

Q_SIGNALS:
    void systemChanged();
    void maxAmountChanged();
    void colorChanged();
    void colorVariationChanged();
    void unifiedColorVariationChanged();
    void fadeInEffectChanged();
    void fadeOutEffectChanged();
    void fadeInDurationChanged();
    void fadeOutDurationChanged();
    void alignModeChanged();
    void alignTargetPositionChanged();

protected:
    // From QQmlParserStatus
    void componentComplete() override;

    virtual void reset() = 0;

    void updateBurstIndex(int amount);
    // This will return the next available index
    int nextCurrentIndex();
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override
    {
        return node;
    }

    QList<QQuick3DParticleData> m_particleData;

    int m_maxAmount = 100;
    int m_currentIndex = -1;
    int m_lastBurstIndex = 0;
    AlignMode m_alignMode = AlignNone;
    QVector3D m_alignTarget;

private:
    friend class QQuick3DParticleSystem;
    friend class QQuick3DParticleEmitter;

    QQuick3DParticleSystem* m_system = nullptr;

    QColor m_color;
    QVector4D m_colorVariation;
    bool m_unifiedColorVariation = false;
    FadeType m_fadeInEffect = FadeOpacity;
    FadeType m_fadeOutEffect = FadeOpacity;
    int m_fadeInDuration = 250;
    int m_fadeOutDuration = 250;
};

QT_END_NAMESPACE

#endif

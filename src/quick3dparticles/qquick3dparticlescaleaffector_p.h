/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef QQUICK3DPARTICLESCALEAFFECTOR_H
#define QQUICK3DPARTICLESCALEAFFECTOR_H

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

#include <QtQuick3DParticles/private/qquick3dparticleaffector_p.h>
#include <qeasingcurve.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleScaleAffector : public QQuick3DParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(float minSize READ minSize WRITE setMinSize NOTIFY minSizeChanged)
    Q_PROPERTY(float maxSize READ maxSize WRITE setMaxSize NOTIFY maxSizeChanged)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(ScalingType type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(QEasingCurve easingCurve READ easingCurve WRITE setEasingCurve NOTIFY easingCurveChanged)
    QML_NAMED_ELEMENT(ScaleAffector3D)
    QML_ADDED_IN_VERSION(6, 4)
public:
    QQuick3DParticleScaleAffector(QQuick3DNode *parent = nullptr);

    enum ScalingType
    {
        Linear,
        SewSaw,
        SineWave,
        AbsSineWave,
        Step,
        SmoothStep,
    };
    Q_ENUM(ScalingType)

    float minSize() const;
    float maxSize() const;
    int duration() const;
    ScalingType type() const;
    QEasingCurve easingCurve() const;

public Q_SLOTS:
    void setMinSize(float size);
    void setMaxSize(float size);
    void setDuration(int duration);
    void setType(ScalingType type);
    void setEasingCurve(const QEasingCurve &curve);

Q_SIGNALS:
    void minSizeChanged();
    void maxSizeChanged();
    void durationChanged();
    void typeChanged();
    void easingCurveChanged();

protected:
    void prepareToAffect() override;
    void affectParticle(const QQuick3DParticleData &, QQuick3DParticleDataCurrent *d, float time) override;

private:
    float m_minSize = 1.0f;
    float m_maxSize = 1.0f;
    int m_duration = 1000;
    ScalingType m_type = Linear;
    QEasingCurve m_easing;
};

QT_END_NAMESPACE

#endif

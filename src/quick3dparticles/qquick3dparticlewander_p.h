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

#ifndef QQUICK3DPARTICLEWANDER_H
#define QQUICK3DPARTICLEWANDER_H

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
#include <QtQuick3DParticles/private/qquick3dparticleaffector_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleWander : public QQuick3DParticleAffector
{
    Q_OBJECT
    // How long distance particle maximally moves. Value n means particle position changing -n .. +n.
    Q_PROPERTY(QVector3D globalAmount READ globalAmount WRITE setGlobalAmount NOTIFY globalAmountChanged)
    // With what pace/frequency particle wandes. Value n means curves / second
    Q_PROPERTY(QVector3D globalPace READ globalPace WRITE setGlobalPace NOTIFY globalPaceChanged)
    Q_PROPERTY(QVector3D uniqueAmount READ uniqueAmount WRITE setUniqueAmount NOTIFY uniqueAmountChanged)
    Q_PROPERTY(QVector3D uniquePace READ uniquePace WRITE setUniquePace NOTIFY uniquePaceChanged)
    // Amount variation between 0.0 - 1.0
    // When amount variation is 0.0, every particle reaches max distance. When 0.5, every particle reaches something between 0.5-1.5 distance.
    // Default 0.0 so no variation.
    Q_PROPERTY(float uniqueAmountVariation READ uniqueAmountVariation WRITE setUniqueAmountVariation NOTIFY uniqueAmountVariationChanged)
    // Pace variation between 0.0 - 1.0
    // When pace variation is 0.0, every particle wander at same frequency. When 0.5, the pace is multipled with something between 0.5 - 1.5.
    // Default 0.0 so no variation.
    Q_PROPERTY(float uniquePaceVariation READ uniquePaceVariation WRITE setUniquePaceVariation NOTIFY uniquePaceVariationChanged)
    QML_NAMED_ELEMENT(Wander3D)

public:
    QQuick3DParticleWander(QObject *parent = nullptr);

    const QVector3D &globalAmount() const;
    const QVector3D &globalPace() const;
    const QVector3D &uniqueAmount() const;
    const QVector3D &uniquePace() const;
    float uniqueAmountVariation() const;
    float uniquePaceVariation() const;

public Q_SLOTS:
    void setGlobalAmount(const QVector3D &globalAmount);
    void setGlobalPace(const QVector3D &globalPace);
    void setUniqueAmount(const QVector3D &uniqueAmount);
    void setUniquePace(const QVector3D &uniquePace);
    void setUniqueAmountVariation(float uniqueAmountVariation);
    void setUniquePaceVariation(float uniquePaceVariation);

protected:
    void affectParticle(const QQuick3DParticleData &sd, QQuick3DParticleDataCurrent *d, float time) override;

Q_SIGNALS:
    void globalAmountChanged();
    void globalPaceChanged();
    void uniqueAmountChanged();
    void uniquePaceChanged();
    void uniqueAmountVariationChanged();
    void uniquePaceVariationChanged();

private:
    QVector3D m_globalAmount;
    QVector3D m_globalPace;
    QVector3D m_uniqueAmount;
    QVector3D m_uniquePace;
    float m_uniqueAmountVariation = 0;
    float m_uniquePaceVariation = 0;
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLEWANDER_H

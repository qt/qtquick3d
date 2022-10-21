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
    Q_PROPERTY(QVector3D globalAmount READ globalAmount WRITE setGlobalAmount NOTIFY globalAmountChanged)
    Q_PROPERTY(QVector3D globalPace READ globalPace WRITE setGlobalPace NOTIFY globalPaceChanged)
    Q_PROPERTY(QVector3D globalPaceStart READ globalPaceStart WRITE setGlobalPaceStart NOTIFY globalPaceStartChanged)
    Q_PROPERTY(QVector3D uniqueAmount READ uniqueAmount WRITE setUniqueAmount NOTIFY uniqueAmountChanged)
    Q_PROPERTY(QVector3D uniquePace READ uniquePace WRITE setUniquePace NOTIFY uniquePaceChanged)
    Q_PROPERTY(float uniqueAmountVariation READ uniqueAmountVariation WRITE setUniqueAmountVariation NOTIFY uniqueAmountVariationChanged)
    Q_PROPERTY(float uniquePaceVariation READ uniquePaceVariation WRITE setUniquePaceVariation NOTIFY uniquePaceVariationChanged)
    Q_PROPERTY(int fadeInDuration READ fadeInDuration WRITE setFadeInDuration NOTIFY fadeInDurationChanged)
    Q_PROPERTY(int fadeOutDuration READ fadeOutDuration WRITE setFadeOutDuration NOTIFY fadeOutDurationChanged)
    QML_NAMED_ELEMENT(Wander3D)
    QML_ADDED_IN_VERSION(6, 2)

public:
    QQuick3DParticleWander(QQuick3DNode *parent = nullptr);

    const QVector3D &globalAmount() const;
    const QVector3D &globalPace() const;
    const QVector3D &globalPaceStart() const;
    const QVector3D &uniqueAmount() const;
    const QVector3D &uniquePace() const;
    float uniqueAmountVariation() const;
    float uniquePaceVariation() const;
    int fadeInDuration() const;
    int fadeOutDuration() const;

public Q_SLOTS:
    void setGlobalAmount(const QVector3D &globalAmount);
    void setGlobalPace(const QVector3D &globalPace);
    void setGlobalPaceStart(const QVector3D &globalPaceStart);
    void setUniqueAmount(const QVector3D &uniqueAmount);
    void setUniquePace(const QVector3D &uniquePace);
    void setUniqueAmountVariation(float uniqueAmountVariation);
    void setUniquePaceVariation(float uniquePaceVariation);
    void setFadeInDuration(int fadeInDuration);
    void setFadeOutDuration(int fadeOutDuration);

Q_SIGNALS:
    void globalAmountChanged();
    void globalPaceChanged();
    void globalPaceStartChanged();
    void uniqueAmountChanged();
    void uniquePaceChanged();
    void uniqueAmountVariationChanged();
    void uniquePaceVariationChanged();
    void fadeInDurationChanged();
    void fadeOutDurationChanged();

protected:
    void affectParticle(const QQuick3DParticleData &sd, QQuick3DParticleDataCurrent *d, float time) override;

private:
    QVector3D m_globalAmount;
    QVector3D m_globalPace;
    QVector3D m_globalPaceStart;
    QVector3D m_uniqueAmount;
    QVector3D m_uniquePace;
    float m_uniqueAmountVariation = 0.0f;
    float m_uniquePaceVariation = 0.0f;
    int m_fadeInDuration = 0;
    int m_fadeOutDuration = 0;
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLEWANDER_H

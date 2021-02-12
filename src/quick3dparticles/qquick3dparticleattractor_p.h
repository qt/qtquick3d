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

#ifndef QQUICK3DPARTICLEATTRACTOR_H
#define QQUICK3DPARTICLEATTRACTOR_H

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
#include <QtQuick3DParticles/private/qquick3dparticleshapenode_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleAttractor : public QQuick3DParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(QVector3D positionVariation READ positionVariation WRITE setPositionVariation NOTIFY positionVariationChanged)
    Q_PROPERTY(QQuick3DParticleShapeNode *shapeNode READ shapeNode WRITE setShapeNode NOTIFY shapeNodeChanged)
    // Time in ms it takes for particles to reach attractor.
    // Default value is -1, meaning duration is as long as particles lifetime.
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(int durationVariation READ durationVariation WRITE setDurationVariation NOTIFY durationVariationChanged)
    // Set to true if you want particles to hide when they reach the affector position
    // Default value false
    Q_PROPERTY(bool hideAtEnd READ hideAtEnd WRITE setHideAtEnd NOTIFY hideAtEndChanged)
    QML_NAMED_ELEMENT(Attractor3D)

public:
    QQuick3DParticleAttractor(QObject *parent = nullptr);

    QVector3D position() const;
    QVector3D positionVariation() const;
    QQuick3DParticleShapeNode *shapeNode() const;
    int duration() const;
    int durationVariation() const;
    bool hideAtEnd() const;

public Q_SLOTS:
    void setPosition(const QVector3D &position);
    void setPositionVariation(const QVector3D &positionVariation);
    void setShapeNode(QQuick3DParticleShapeNode *shapeNode);
    void setDuration(int duration);
    void setDurationVariation(int durationVariation);
    void setHideAtEnd(bool hideAtEnd);

protected:
    void affectParticle(const QQuick3DParticleData &sd, QQuick3DParticleDataCurrent *d, float time) override;

Q_SIGNALS:
    void positionChanged();
    void positionVariationChanged();
    void shapeNodeChanged();
    void durationChanged();
    void durationVariationChanged();
    void hideAtEndChanged();

private:
    void updateShapePositions();

    QVector3D m_position;
    QQuick3DParticleShapeNode *m_shapeNode = nullptr;
    QList<QVector3D> m_shapePositionList;
    bool m_shapeDirty = false;
    int m_duration = -1;
    int m_durationVariation = 0;
    QVector3D m_positionVariation;
    bool m_hideAtEnd = false;
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLEATTRACTOR_H

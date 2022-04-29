/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QQUICK3DPARTICLEPOINTROTATOR_H
#define QQUICK3DPARTICLEPOINTROTATOR_H

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
#include <QtQuick3DParticles/private/qquick3dparticleaffector_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticlePointRotator : public QQuick3DParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(float magnitude READ magnitude WRITE setMagnitude NOTIFY magnitudeChanged)
    Q_PROPERTY(QVector3D direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(QVector3D pivotPoint READ pivotPoint WRITE setPivotPoint NOTIFY pivotPointChanged)
    QML_NAMED_ELEMENT(PointRotator3D)
    QML_ADDED_IN_VERSION(6, 2)

public:
    QQuick3DParticlePointRotator(QQuick3DNode *parent = nullptr);

    float magnitude() const;
    QVector3D direction() const;
    QVector3D pivotPoint() const;

public Q_SLOTS:
    void setMagnitude(float magnitude);
    void setDirection(const QVector3D &direction);
    void setPivotPoint(const QVector3D &point);

Q_SIGNALS:
    void magnitudeChanged();
    void directionChanged();
    void pivotPointChanged();

protected:
    void prepareToAffect() override;
    void affectParticle(const QQuick3DParticleData &sd, QQuick3DParticleDataCurrent *d, float time) override;

private:
    float m_magnitude = 10.0f;
    QVector3D m_direction = {0.0f, 1.0f, 0.0f};
    QVector3D m_directionNormalized = {0.0f, 1.0f, 0.0f};
    QVector3D m_pivotPoint;
    QMatrix4x4 m_rotationMatrix;
};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLEPOINTROTATOR_H

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

#ifndef QQUICK3DPARTICLETARGETDIRECTION_H
#define QQUICK3DPARTICLETARGETDIRECTION_H

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

#include <QVector3D>
#include <QQmlEngine>

#include <QtQuick3DParticles/private/qquick3dparticledirection_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DPARTICLES_EXPORT QQuick3DParticleTargetDirection : public QQuick3DParticleDirection
{
    Q_OBJECT
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(QVector3D positionVariation READ positionVariation WRITE setPositionVariation NOTIFY positionVariationChanged)
    Q_PROPERTY(bool normalized READ normalized WRITE setNormalized NOTIFY normalizedChanged)
    Q_PROPERTY(float magnitude READ magnitude WRITE setMagnitude NOTIFY magnitudeChanged)
    Q_PROPERTY(float magnitudeVariation READ magnitudeVariation WRITE setMagnitudeVariation NOTIFY magnitudeChangedVariation)
    QML_NAMED_ELEMENT(TargetDirection3D)
    QML_ADDED_IN_VERSION(6, 2)

public:
    QQuick3DParticleTargetDirection(QObject *parent = nullptr);

    QVector3D position() const;
    void setPosition(const QVector3D &position);
    bool normalized() const;
    float magnitude() const;
    float magnitudeVariation() const;
    QVector3D positionVariation() const;

public Q_SLOTS:
    void setPositionVariation(const QVector3D &positionVariation);
    void setNormalized(bool normalized);
    void setMagnitude(float magnitude);
    void setMagnitudeVariation(float magnitudeVariation);

Q_SIGNALS:
    void positionChanged();
    void positionVariationChanged();
    void normalizedChanged();
    void magnitudeChanged();
    void magnitudeChangedVariation();

private:
    QVector3D sample(const QQuick3DParticleData &d) override;
    QVector3D m_position;
    QVector3D m_positionVariation;
    bool m_normalized = false;
    float m_magnitude = 1.0f;
    float m_magnitudeVariation = 0.0f;

};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLETARGETDIRECTION_H

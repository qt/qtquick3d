// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRHANDTRACKERINPUT_H
#define QOPENXRHANDTRACKERINPUT_H

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
#include <QVector2D>
#include <QVector3D>
#include <QQuaternion>
#include <QtQml/qqml.h>
#include <private/qquick3dmodel_p.h>

#include <QQuick3DGeometry>

#include <QtQuick3DXr/private/qtquick3dxrglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuick3DXrHandTrackerInput;

class QQuick3DXrHandTrackerInputPrivate
{
    Q_DECLARE_PUBLIC(QQuick3DXrHandTrackerInput)

public:
    using HandPoseSpace = QtQuick3DXr::HandPoseSpace;

    explicit QQuick3DXrHandTrackerInputPrivate(QQuick3DXrHandTrackerInput &handTrackerInput);

    bool isActive() const;
    HandPoseSpace poseSpace() const;
    void setIsActive(bool isActive);

    const QVector3D &posePosition() const;
    const QQuaternion &poseRotation() const;

    QList<QVector3D> jointPositions() const;
    void setJointPositionsAndRotations(const QList<QVector3D> &newJointPositions, const QList<QQuaternion> &newJointRotations);

    QList<QQuaternion> jointRotations() const;

    QVector3D pokePosition() const;
    void setPokePosition(const QVector3D &newPokePosition);

    void setPoseSpace(HandPoseSpace poseSpace);

private:
    QQuick3DXrHandTrackerInput *q_ptr = nullptr;

    bool m_isActive = false;
    HandPoseSpace m_poseSpace = HandPoseSpace::GripPose;

    QVector3D m_posePosition;
    QQuaternion m_poseRotation;

    QList<QVector3D> m_jointPositions;
    QList<QQuaternion> m_jointRotations;
    QVector3D m_pokePosition;
};

QT_END_NAMESPACE

#endif // QOPENXRHANDTRACKERINPUT_H

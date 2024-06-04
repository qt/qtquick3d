// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DXRHANDINPUT_P_H
#define QQUICK3DXRHANDINPUT_P_H

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

QT_BEGIN_NAMESPACE

class QQuick3DXrHandInput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isActive READ isActive NOTIFY isActiveChanged)
    Q_PROPERTY(HandPoseSpace poseSpace READ poseSpace WRITE setPoseSpace NOTIFY poseSpaceChanged)
    Q_PROPERTY(QVector3D posePosition READ posePosition NOTIFY posePositionChanged)
    Q_PROPERTY(QQuaternion poseRotation READ poseRotation NOTIFY poseRotationChanged)

    QML_NAMED_ELEMENT(XrHandInput)
    QML_UNCREATABLE("Created by XrView")
    QML_ADDED_IN_VERSION(6, 8)

public:
    enum class HandPoseSpace {
        GripPose,
        AimPose
    };
    Q_ENUM(HandPoseSpace)


    explicit QQuick3DXrHandInput(QObject *parent = nullptr);

    bool isActive() const;
    HandPoseSpace poseSpace() const;
    void setIsActive(bool isActive);
    void setPosePosition(const QVector3D &position);
    void setPoseRotation(const QQuaternion &rotation);

    const QVector3D &posePosition() const;

    const QQuaternion &poseRotation() const;

    void setInputValue(int id, const char *shortName, float value) { emit inputValueChange(id, shortName, value); }

public Q_SLOTS:
    void setPoseSpace(HandPoseSpace poseSpace);

Q_SIGNALS:
    void isActiveChanged();

    void inputValueChange(int id, const char *shortName, float value);

    void poseSpaceChanged();
    void posePositionChanged();
    void poseRotationChanged();

private:
    bool m_isActive = false;
    HandPoseSpace m_poseSpace = HandPoseSpace::GripPose;
    QVector3D m_posePosition;
    QQuaternion m_poseRotation;
};

QT_END_NAMESPACE

#endif // QQUICK3DXRHANDINPUT_P_H

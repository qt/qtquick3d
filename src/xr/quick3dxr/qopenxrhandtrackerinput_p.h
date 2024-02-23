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

QT_BEGIN_NAMESPACE

class QOpenXRHandTrackerInput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isActive READ isActive NOTIFY isActiveChanged)
    Q_PROPERTY(HandPoseSpace poseSpace READ poseSpace WRITE setPoseSpace NOTIFY poseSpaceChanged)

    Q_PROPERTY(QVector3D posePosition READ posePosition NOTIFY posePositionChanged)

    Q_PROPERTY(QList<QVector3D> jointPositions READ jointPositions NOTIFY jointPositionsChanged FINAL)
    Q_PROPERTY(QList<QQuaternion> jointRotations READ jointRotations NOTIFY jointRotationsChanged FINAL)

    QML_NAMED_ELEMENT(XrHandTrackerInput)
    QML_UNCREATABLE("Created by XrView")

public:
    enum class HandPoseSpace {
        GripPose,
        AimPose,
        PinchPose,
        PokePose
    };
    Q_ENUM(HandPoseSpace)

    explicit QOpenXRHandTrackerInput(QObject *parent = nullptr);

    bool isActive() const;
    HandPoseSpace poseSpace() const;
    void setIsActive(bool isActive);

    const QVector3D &posePosition() const;
    const QQuaternion &poseRotation() const;

    QList<QVector3D> jointPositions() const;
    void setJointPositionsAndRotations(const QList<QVector3D> &newJointPositions, const QList<QQuaternion> &newJointRotations);

    QList<QQuaternion> jointRotations() const;

public Q_SLOTS:
    void setPoseSpace(HandPoseSpace poseSpace);

Q_SIGNALS:
    void isActiveChanged();
    void poseSpaceChanged();

    void posePositionChanged();
    void poseRotationChanged();

    void jointPositionsChanged();
    void jointDataUpdated();

    void jointRotationsChanged();

private:
    bool m_isActive = false;
    HandPoseSpace m_poseSpace = HandPoseSpace::GripPose;

    QVector3D m_posePosition;
    QQuaternion m_poseRotation;

    QList<QVector3D> m_jointPositions;
    QList<QQuaternion> m_jointRotations;
};

class QOpenXrHandModel : public QQuick3DModel
{
    Q_OBJECT

    Q_PROPERTY(QOpenXRHandTrackerInput *handTracker READ handTracker WRITE setHandTracker NOTIFY handTrackerChanged FINAL)

    QML_NAMED_ELEMENT(XrHandModel)

public:
    QOpenXrHandModel(QQuick3DNode *parent = nullptr);

    void componentComplete() override;

    QOpenXRHandTrackerInput *handTracker() const;
    void setHandTracker(QOpenXRHandTrackerInput *newHandTracker);

Q_SIGNALS:
    void handChanged();
    void handTrackerChanged();

private Q_SLOTS:
    void updatePose();

private:
    void setupModel();
    QOpenXRHandTrackerInput *m_handTracker = nullptr;
    bool m_initialized = false;
};

QT_END_NAMESPACE

#endif // QOPENXRHANDTRACKERINPUT_H
